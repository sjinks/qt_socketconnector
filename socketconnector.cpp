#include <QtNetwork/QHostAddress>
#include <sys/socket.h>
#include "socketconnector.h"
#include "socketconnector_p.h"

SocketConnector::SocketConnector(QObject* parent)
	: QObject(parent), d_ptr(new SocketConnectorPrivate(this))
{
}

SocketConnector::~SocketConnector(void)
{
}

bool SocketConnector::createSocket(int domain, int type, int proto)
{
	Q_D(SocketConnector);
	return d->createSocket(domain, type, proto);
}

bool SocketConnector::createTcpSocket(void)
{
	return this->createSocket(AF_INET, SOCK_STREAM, 0);
}

bool SocketConnector::createUdpSocket(void)
{
	return this->createSocket(AF_INET, SOCK_DGRAM, 0);
}

bool SocketConnector::bindTo(const QHostAddress& a, quint16 port)
{
	Q_D(SocketConnector);
	return d->bindTo(a, port);
}

void SocketConnector::connectToHost(const QString& address, quint16 port)
{
	Q_D(SocketConnector);
	d->connectToHost(address, port);
}

void SocketConnector::connectToHost(const QHostAddress& address, quint16 port)
{
	this->connectToHost(address.toString(), port);
}

void SocketConnector::disconnectFromHost(void)
{
	Q_D(SocketConnector);
	d->disconnectFromHost();
}

void SocketConnector::abort(void)
{
	Q_D(SocketConnector);
	d->abort();
}

bool SocketConnector::waitForConnected(int timeout)
{
	Q_D(SocketConnector);
	return d->waitForConnected(timeout);
}

bool SocketConnector::assignTo(QAbstractSocket* target)
{
	Q_D(SocketConnector);
	int fd = d->m_fd;

	if (QAbstractSocket::ConnectedState == d->m_state) {
		bool res = target->setSocketDescriptor(fd, QAbstractSocket::ConnectedState, QIODevice::ReadWrite);
		if (res) {
			d->m_fd = -1;
		}

		return true;
	}

	return false;
}

QAbstractSocket::SocketType SocketConnector::socketType(void) const
{
	Q_D(const SocketConnector);
	int domain = d->m_domain;
	int type   = d->m_type;

	if (domain != AF_INET && domain != AF_INET6) {
		return QAbstractSocket::UnknownSocketType;
	}

	if (SOCK_STREAM == type) {
		return QAbstractSocket::TcpSocket;
	}

	if (SOCK_DGRAM == type) {
		return QAbstractSocket::UdpSocket;
	}

	return QAbstractSocket::UnknownSocketType;
}

QAbstractSocket::SocketState SocketConnector::state(void) const
{
	Q_D(const SocketConnector);
	return d->m_state;
}

QAbstractSocket::SocketError SocketConnector::error(void) const
{
	Q_D(const SocketConnector);
	return d->m_error;
}


#include "moc_socketconnector.cpp"
