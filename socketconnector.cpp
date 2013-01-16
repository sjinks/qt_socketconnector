#include <qplatformdefs.h>
#include <QtCore/QSocketNotifier>
#include <QtCore/QTimer>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QHostInfo>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <errno.h>
#include "socketconnector.h"

SocketConnector::SocketConnector(QObject* parent)
	: QObject(parent), m_fd(-1), m_domain(-1), m_type(-1), m_proto(-1), m_port(0), m_state(QAbstractSocket::UnconnectedState),
	  m_notifier(0), m_timer(0)
{
}

SocketConnector::~SocketConnector(void)
{
	delete this->m_timer;
	delete this->m_notifier;
	if (-1 != this->m_fd) {
		::close(this->m_fd);
	}
}

bool SocketConnector::createSocket(int domain, int type, int proto)
{
	qDebug("%s", Q_FUNC_INFO);
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

bool SocketConnector::bindTo(const QHostAddress& a, quint16 port)
{
	qDebug("%s: %s:%d", Q_FUNC_INFO, qPrintable(a.toString()), int(port));
	if (-1 == this->m_fd || a.isNull()) {
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

	if (res) {
		this->m_state = QAbstractSocket::BoundState;
		this->m_bound_address = a;
		this->m_bound_port    = port;
		Q_EMIT this->stateChanged(this->m_state);
		return true;
	}

	Q_EMIT this->error(QAbstractSocket::UnknownSocketError);
	return false;
}

void SocketConnector::connectToHost(const QString& address, quint16 port)
{
	qDebug("%s %s:%d", Q_FUNC_INFO, qPrintable(address), int(port));
	if (this->m_state == QAbstractSocket::ConnectingState || this->m_state == QAbstractSocket::ConnectedState || this->m_state == QAbstractSocket::HostLookupState || this->m_state == QAbstractSocket::ClosingState) {
		qWarning("%s called when already looking up or connecting/connected to \"%s\"", Q_FUNC_INFO, qPrintable(address));
		return;
	}

	this->m_port = port;

	this->m_state = QAbstractSocket::HostLookupState;
	Q_EMIT this->stateChanged(this->m_state);

	QHostAddress tmp;
	if (tmp.setAddress(address)) {
		QHostInfo info;
		info.setAddresses(QList<QHostAddress>() << tmp);
		this->_q_startConnecting(info);
	}
	else {
		QHostInfo::lookupHost(address, this, SLOT(_q_startConnecting(QHostInfo)));
	}
}

void SocketConnector::connectToHost(const QHostAddress& address, quint16 port)
{
	this->connectToHost(address.toString(), port);
}

void SocketConnector::disconnectFromHost(void)
{
	qDebug("%s", Q_FUNC_INFO);
	QAbstractSocket::SocketState prev = this->m_state;

	if (-1 != this->m_fd) {
		this->m_state = QAbstractSocket::ClosingState;
		Q_EMIT this->stateChanged(this->m_state);

		::close(this->m_fd);
	}

	if (QAbstractSocket::UnconnectedState != this->m_state) {
		this->m_state = QAbstractSocket::UnconnectedState;
		Q_EMIT this->stateChanged(this->m_state);
	}

	if (QAbstractSocket::ConnectedState == prev) {
		Q_EMIT this->disconnected();
	}

	delete this->m_notifier;
	delete this->m_timer;
	this->m_notifier = 0;
	this->m_timer    = 0;
	this->m_addresses.clear();
	this->m_fd = -1;
	this->m_bound_address.clear();
}

bool SocketConnector::assignTo(QAbstractSocket* target)
{
	if (QAbstractSocket::ConnectedState == this->m_state) {
		bool res = target->setSocketDescriptor(this->m_fd, QAbstractSocket::ConnectedState, QIODevice::ReadWrite);
		if (res) {
			this->m_fd = -1;
		}

		return true;
	}

	return false;
}

bool SocketConnector::bindV4(const QHostAddress& a, quint16 port)
{
	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));

	sa.sin_family      = AF_INET;
	sa.sin_port        = htons(port);
	sa.sin_addr.s_addr = htonl(a.toIPv4Address());

	return -1 != ::bind(this->m_fd, reinterpret_cast<struct sockaddr*>(&sa), sizeof(sa));
}

bool SocketConnector::bindV6(const QHostAddress& a, quint16 port)
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

int SocketConnector::connectV4(const QHostAddress& a)
{
	qDebug("%s", Q_FUNC_INFO);
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

int SocketConnector::connectV6(const QHostAddress& a)
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


void SocketConnector::_q_startConnecting(const QHostInfo& info)
{
	qDebug("%s", Q_FUNC_INFO);
	this->m_addresses = info.addresses();

	if (this->m_addresses.isEmpty()) {
		this->m_state = QAbstractSocket::UnconnectedState;
		Q_EMIT this->stateChanged(this->m_state);
		Q_EMIT this->error(QAbstractSocket::HostNotFoundError);
		return;
	}

	this->m_state = QAbstractSocket::ConnectingState;
	Q_EMIT this->stateChanged(this->m_state);
	Q_EMIT this->hostFound();
	this->_q_connectToNextAddress();
}

void SocketConnector::_q_connectToNextAddress(void)
{
	qDebug("%s", Q_FUNC_INFO);
	if (this->m_addresses.isEmpty()) {
		this->m_state = QAbstractSocket::UnconnectedState;
		Q_EMIT this->stateChanged(this->m_state);
		Q_EMIT this->error(QAbstractSocket::ConnectionRefusedError);
		return;
	}

	qDebug("%s %s", Q_FUNC_INFO, qPrintable(this->m_addresses.first().toString()));

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
			QMetaObject::invokeMethod(this, "_q_connectToNextAddress", Qt::QueuedConnection);
			return;
	}

	if (!res) {
		this->m_state = QAbstractSocket::ConnectedState;
		Q_EMIT this->stateChanged(this->m_state);
		Q_EMIT this->connected();
		return;
	}

	if (errno == EINPROGRESS) {
		delete this->m_notifier;
		delete this->m_timer;
		this->m_notifier = new QSocketNotifier(this->m_fd, QSocketNotifier::Write, this);
		QObject::connect(this->m_notifier, SIGNAL(activated(int)), this, SLOT(_q_connected(int)));
		this->m_notifier->setEnabled(true);

		this->m_timer = new QTimer(this);
		this->m_timer->setSingleShot(true);
		QObject::connect(this->m_timer, SIGNAL(timeout()), this, SLOT(_q_abortConnection()));
		this->m_timer->start(30000);
		return;
	}

	QMetaObject::invokeMethod(this, "_q_connectToNextAddress", Qt::QueuedConnection);
}

void SocketConnector::_q_connected(int sock)
{
	this->m_notifier->setEnabled(false);

	qDebug("%s", Q_FUNC_INFO);
	int err;
	socklen_t l = sizeof(err);
	getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &l);
	qDebug("%d", err);

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
	Q_EMIT this->stateChanged(this->m_state);
	Q_EMIT this->connected();
}

void SocketConnector::_q_abortConnection(void)
{
	qDebug("%s", Q_FUNC_INFO);
	this->m_notifier->setEnabled(false);
	delete this->m_notifier;
	this->m_timer->deleteLater();

	this->m_timer    = 0;
	this->m_notifier = 0;

	if (-1 != this->m_fd) {
		::close(this->m_fd);
	}

	this->m_fd = this->recreateSocket();
	QMetaObject::invokeMethod(this, "_q_connectToNextAddress", Qt::QueuedConnection);
}

int SocketConnector::recreateSocket(void)
{
	qDebug("%s", Q_FUNC_INFO);
	int fd = ::socket(this->m_domain, this->m_type, this->m_proto);
	if (fd != -1) {
		::fcntl(fd, F_SETFL, ::fcntl(fd, F_GETFL) | O_NONBLOCK);

		if (!this->m_bound_address.isNull()) {
			this->bindTo(this->m_bound_address, this->m_bound_port);
		}
	}

	return fd;
}
