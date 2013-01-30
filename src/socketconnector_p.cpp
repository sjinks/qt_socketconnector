#include <QtCore/QEventLoop>
#include <QtCore/QSocketNotifier>
#include <QtCore/QTimer>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <errno.h>
#include "socketconnector.h"
#include "socketconnector_p.h"

SocketConnectorPrivate::SocketConnectorPrivate(SocketConnector* const q)
	: q_ptr(q), m_fd(-1), m_domain(-1), m_type(-1), m_proto(-1), m_port(0),
	  m_state(QAbstractSocket::UnconnectedState), m_error(QAbstractSocket::UnknownSocketError),
	  m_lookup_id(-1), m_notifier(0), m_timer(0)
{
}

SocketConnectorPrivate::~SocketConnectorPrivate(void)
{
	delete this->m_timer;
	delete this->m_notifier;
	if (-1 != this->m_fd) {
		::close(this->m_fd);
	}
}

bool SocketConnectorPrivate::createSocket(int domain, int type, int proto)
{
	if (QAbstractSocket::UnconnectedState != this->m_state) {
		return false;
	}

	this->m_domain = domain;
	this->m_type   = type;
	this->m_proto  = proto;

	this->disconnectFromHost();
	this->m_fd = this->recreateSocket();

	return this->m_fd != -1;
}

bool SocketConnectorPrivate::bindTo(const QHostAddress& a, quint16 port)
{
	if (-1 == this->m_fd) {
		qWarning("%s: call SocketConnector::createSocket() first", Q_FUNC_INFO);
		return false;
	}

	if (a.isNull()) {
		qWarning("%s: cannot bind to the empty address", Q_FUNC_INFO);
		return false;
	}

	bool res;
	switch (a.protocol()) {
		case QAbstractSocket::IPv4Protocol:
			res = this->bindV4(a, port);
			break;

		case QAbstractSocket::IPv6Protocol:
			res = this->bindV6(a, port);
			break;

		default:
			res = false;
			break;
	}

	Q_Q(SocketConnector);
	if (res) {
		this->m_state = QAbstractSocket::BoundState;
		this->m_bound_address = a;
		this->m_bound_port    = port;
		Q_EMIT q->stateChanged(this->m_state);
		return true;
	}

	this->m_error = QAbstractSocket::UnknownSocketError;
	Q_EMIT q->error(this->m_error);
	return false;
}

void SocketConnectorPrivate::connectToHost(const QString& address, quint16 port)
{
	if (this->m_state == QAbstractSocket::ConnectingState || this->m_state == QAbstractSocket::ConnectedState || this->m_state == QAbstractSocket::HostLookupState || this->m_state == QAbstractSocket::ClosingState) {
		qWarning("%s called when already looking up or connecting/connected to \"%s\"", Q_FUNC_INFO, qPrintable(address));
		return;
	}

	Q_Q(SocketConnector);

	this->m_port  = port;
	this->m_state = QAbstractSocket::HostLookupState;
	Q_EMIT q->stateChanged(this->m_state);

	QHostAddress tmp;
	if (tmp.setAddress(address)) {
		QHostInfo info;
		info.setAddresses(QList<QHostAddress>() << tmp);
		this->_q_startConnecting(info);
	}
	else {
		this->m_lookup_id = QHostInfo::lookupHost(address, q, SLOT(_q_startConnecting(QHostInfo)));
	}
}

void SocketConnectorPrivate::disconnectFromHost(void)
{
	Q_Q(SocketConnector);

	QAbstractSocket::SocketState prev = this->m_state;

	if (-1 != this->m_fd) {
		this->m_state = QAbstractSocket::ClosingState;
		Q_EMIT q->stateChanged(this->m_state);

		::close(this->m_fd);
	}

	if (QAbstractSocket::UnconnectedState != this->m_state) {
		this->m_state = QAbstractSocket::UnconnectedState;
		Q_EMIT q->stateChanged(this->m_state);
	}

	if (QAbstractSocket::ConnectedState == prev) {
		Q_EMIT q->disconnected();
	}

	delete this->m_notifier;
	delete this->m_timer;
	this->m_notifier = 0;
	this->m_timer    = 0;
	this->m_addresses.clear();
	this->m_fd = -1;
	this->m_bound_address.clear();
}

void SocketConnectorPrivate::abort(void)
{
	if (QAbstractSocket::UnconnectedState == this->m_state) {
		return;
	}

	if (-1 != this->m_lookup_id) {
		QHostInfo::abortHostLookup(this->m_lookup_id);
		this->m_lookup_id = -1;
	}

	this->disconnectFromHost();
}

bool SocketConnectorPrivate::waitForConnected(int timeout)
{
	Q_Q(SocketConnector);

	if (QAbstractSocket::ConnectedState == this->m_state) {
		return true;
	}

	QEventLoop loop;
	QObject::connect(q, SIGNAL(connected()), &loop, SLOT(quit()));
	QObject::connect(q, SIGNAL(error(QAbstractSocket::SocketError)), &loop, SLOT(quit()));

	if (timeout != -1) {
		QTimer::singleShot(timeout, &loop, SLOT(quit()));
	}

	loop.exec();

	switch (this->m_state) {
		case QAbstractSocket::ConnectedState:
			return true;

		default:
			this->abort();
			return false;
	}
}

bool SocketConnectorPrivate::bindV4(const QHostAddress& a, quint16 port)
{
	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));

	sa.sin_family      = AF_INET;
	sa.sin_port        = htons(port);
	sa.sin_addr.s_addr = htonl(a.toIPv4Address());

	return -1 != ::bind(this->m_fd, reinterpret_cast<struct sockaddr*>(&sa), sizeof(sa));
}

bool SocketConnectorPrivate::bindV6(const QHostAddress& a, quint16 port)
{
	struct sockaddr_in6 sa;
	memset(&sa, 0, sizeof(sa));

	sa.sin6_family   = AF_INET6;
	sa.sin6_port     = htons(port);
#ifndef QT_NO_IPV6IFNAME
	sa.sin6_scope_id = ::if_nametoindex(a.scopeId().toLatin1().data());
#else
	sa.sin6_scope_id = a.scopeId().toInt();
#endif

	Q_IPV6ADDR tmp = a.toIPv6Address();
	memcpy(&sa.sin6_addr.s6_addr, &tmp, sizeof(tmp));

	return -1 != ::bind(this->m_fd, reinterpret_cast<struct sockaddr*>(&sa), sizeof(sa));
}

int SocketConnectorPrivate::connectV4(const QHostAddress& a)
{
	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));

	sa.sin_family      = AF_INET;
	sa.sin_port        = htons(this->m_port);
	sa.sin_addr.s_addr = htonl(a.toIPv4Address());

	int res;
	do {
		res = ::connect(this->m_fd, reinterpret_cast<struct sockaddr*>(&sa), sizeof(sa));
	} while (-1 == res && EINTR == errno);

	return res;
}

int SocketConnectorPrivate::connectV6(const QHostAddress& a)
{
	struct sockaddr_in6 sa;
	memset(&sa, 0, sizeof(sa));

	sa.sin6_family   = AF_INET6;
	sa.sin6_port     = htons(this->m_port);
#ifndef QT_NO_IPV6IFNAME
	sa.sin6_scope_id = ::if_nametoindex(a.scopeId().toLatin1().data());
#else
	sa.sin6_scope_id = a.scopeId().toInt();
#endif

	Q_IPV6ADDR tmp = a.toIPv6Address();
	memcpy(&sa.sin6_addr.s6_addr, &tmp, sizeof(tmp));

	int res;
	do {
		res = ::connect(this->m_fd, reinterpret_cast<struct sockaddr*>(&sa), sizeof(sa));
	} while (-1 == res && EINTR == errno);

	return res;
}

void SocketConnectorPrivate::_q_startConnecting(const QHostInfo& info)
{
	this->m_addresses = info.addresses();
	this->m_lookup_id = -1;

	Q_Q(SocketConnector);

	if (this->m_addresses.isEmpty()) {
		this->m_state = QAbstractSocket::UnconnectedState;
		this->m_error = QAbstractSocket::HostNotFoundError;
		Q_EMIT q->stateChanged(this->m_state);
		Q_EMIT q->error(this->m_error);
		return;
	}

	this->m_state = QAbstractSocket::ConnectingState;
	Q_EMIT q->stateChanged(this->m_state);
	Q_EMIT q->hostFound();
	this->_q_connectToNextAddress();
}

void SocketConnectorPrivate::_q_connectToNextAddress(void)
{
	Q_Q(SocketConnector);

	if (this->m_addresses.isEmpty()) {
		this->m_state = QAbstractSocket::UnconnectedState;
		this->m_error = QAbstractSocket::ConnectionRefusedError;
		Q_EMIT q->stateChanged(this->m_state);
		Q_EMIT q->error(this->m_error);
		return;
	}

	int res;
	QHostAddress address = this->m_addresses.takeFirst();
	switch (address.protocol()) {
		case QAbstractSocket::IPv4Protocol:
			res = this->connectV4(address);
			break;

		case QAbstractSocket::IPv6Protocol:
			res = this->connectV6(address);
			break;

		default:
			QMetaObject::invokeMethod(q, "_q_connectToNextAddress", Qt::QueuedConnection);
			return;
	}

	if (!res) {
		this->m_state = QAbstractSocket::ConnectedState;
		Q_EMIT q->stateChanged(this->m_state);
		Q_EMIT q->connected();
		return;
	}

	if (errno == EINPROGRESS) {
		delete this->m_notifier;
		delete this->m_timer;
		this->m_notifier = new QSocketNotifier(this->m_fd, QSocketNotifier::Write, q);
		QObject::connect(this->m_notifier, SIGNAL(activated(int)), q, SLOT(_q_connected(int)));
		this->m_notifier->setEnabled(true);

		this->m_timer = new QTimer(q);
		this->m_timer->setSingleShot(true);
		QObject::connect(this->m_timer, SIGNAL(timeout()), q, SLOT(_q_abortConnection()));
		this->m_timer->start(30000);
		return;
	}

	QMetaObject::invokeMethod(q, "_q_connectToNextAddress", Qt::QueuedConnection);
}

void SocketConnectorPrivate::_q_connected(int sock)
{
	this->m_notifier->setEnabled(false);

	int err;
	socklen_t l = sizeof(err);
	getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &l);

	if (err) {
		this->_q_abortConnection();
		return;
	}

	this->m_notifier->deleteLater();
	delete this->m_timer;

	this->m_timer    = 0;
	this->m_notifier = 0;

	this->m_addresses.clear();

	this->m_state = QAbstractSocket::ConnectedState;

	Q_Q(SocketConnector);
	Q_EMIT q->stateChanged(this->m_state);
	Q_EMIT q->connected();
}

void SocketConnectorPrivate::_q_abortConnection(void)
{
	this->m_notifier->setEnabled(false);
	delete this->m_notifier;
	this->m_timer->deleteLater();

	this->m_timer    = 0;
	this->m_notifier = 0;

	if (-1 != this->m_fd) {
		::close(this->m_fd);
	}

	this->m_fd = this->recreateSocket();
	Q_Q(SocketConnector);
	QMetaObject::invokeMethod(q, "_q_connectToNextAddress", Qt::QueuedConnection);
}

int SocketConnectorPrivate::recreateSocket(void)
{
	int fd = ::socket(this->m_domain, this->m_type, this->m_proto);
	if (fd != -1) {
		::fcntl(fd, F_SETFL, ::fcntl(fd, F_GETFL) | O_NONBLOCK);

		if (!this->m_bound_address.isNull()) {
			this->bindTo(this->m_bound_address, this->m_bound_port);
		}
	}

	return fd;
}
