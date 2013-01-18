#include <QtNetwork/QHostAddress>
#include <sys/socket.h>
#include "socketconnector.h"
#include "socketconnector_p.h"

/**
 * @class SocketConnector
 *
 * @brief The SocketConnector class provides a workaround for QTBUG-27678
 *
 * @see https://bugreports.qt-project.org/browse/QTBUG-27678
 */

/**
 * @fn void SocketConnector::hostFound()
 *
 * This signal is emitted after connectToHost() has been called and
 * the host lookup has succeeded.
 *
 * hostFound() may be emitted directly from the connectToHost() call
 * since a DNS result could have been cached.
 *
 * @sa connected()
 */

/**
 * @fn void SocketConnector::connected()
 *
 * This signal is emitted after connectToHost() has been called and
 * a connection has been successfully established.
 *
 * @note On some operating systems the connected() signal may
 * be directly emitted from the connectToHost() call for connections
 * to the localhost.
 *
 * @sa connectToHost(), disconnected()
 */

/**
 * @fn void SocketConnector::disconnected()
 *
 * This signal is emitted when the socket has been disconnected.
 *
 * @warning If you need to delete the sender() of this signal in a slot connected
 * to it, use the @c deleteLater() function.
 *
 * @sa connectToHost(), disconnectFromHost(), abort()
 */

/**
 * @fn void SocketConnector::error(QAbstractSocket::SocketError socketError)
 *
 * This signal is emitted after an error occurred. The @a socketError
 * parameter describes the type of error that occurred.
 *
 * QAbstractSocket::SocketError is not a registered metatype, therefore for queued
 * connections, you will have to register it with Q_DECLARE_METATYPE() and
 * qRegisterMetaType().
 *
 * @sa error()
 */

/**
 * @fn void SocketConnector::stateChanged(QAbstractSocket::SocketState socketState)
 *
 * This signal is emitted whenever SocketConnector's state changes.
 * The @a socketState parameter is the new state.
 *
 * QAbstractSocket::SocketState is not a registered metatype, therefore for queued
 * connections, you will have to register it with Q_DECLARE_METATYPE() and
 * qRegisterMetaType().
 *
 * @sa state()
 */

/**
 * @brief Creates a new @c SocketConnector
 * @param parent Object parent
 *
 * The @a parent argument is passed to QObject's constructor.
 */
SocketConnector::SocketConnector(QObject* parent)
	: QObject(parent), d_ptr(new SocketConnectorPrivate(this))
{
}

/**
 * @brief Destroys the @c SocketConnector
 */
SocketConnector::~SocketConnector(void)
{
}

/**
 * @brief Creates a new socket and assigns it to the @c SocketConnector.
 * @param domain Socket domain (like @c AF_INET)
 * @param type Socket type (like @c SOCK_STREAM, @c SOCK_DGRAM)
 * @param proto Socket protocol
 * @return Whether the socket was created successfully
 * @see state()
 *
 * The socket can only be assigned to @c SocketConnector if the current state is @c QAbstractSocket::UnconnectedState
 */
bool SocketConnector::createSocket(int domain, int type, int proto)
{
	Q_D(SocketConnector);
	return d->createSocket(domain, type, proto);
}

/**
 * @brief Creates a new TCP socket and assigns it to the @c SocketConnector.
 * @return
 * @see createSocket()
 *
 * This function is the same as @code createSocket(AF_INET, SOCK_STREAM, 0) @endcode
 */
bool SocketConnector::createTcpSocket(void)
{
	return this->createSocket(AF_INET, SOCK_STREAM, 0);
}

/**
 * @brief Creates a new UDP socket and assigns it to the @c SocketConnector.
 * @return
 * @see createSocket()
 *
 * This function is the same as @code createSocket(AF_INET, SOCK_DGRAM, 0) @endcode
 */
bool SocketConnector::createUdpSocket(void)
{
	return this->createSocket(AF_INET, SOCK_DGRAM, 0);
}

/**
 * @brief Binds to address @a a on port @a port
 * @param a Address to bind to
 * @param port Port (in native byte order) to bind to (if 0 or unspecified, a random port is chosen)
 * @return Whether the operation succeeded
 *
 * The socket can be bound to an address if the current state is @c QAbstractSocket::UnconnectedState.
 * It may be an error to call @c bindTo() again after a successful call to @c bindTo().
 * On success, the functions returns @c true and the socket enters @c BoundState; otherwise it returns @c false.
 */
bool SocketConnector::bindTo(const QHostAddress& a, quint16 port)
{
	Q_D(SocketConnector);
	return d->bindTo(a, port);
}

/**
 * @brief Attempts to make a connection to @a address on the given @a port.
 * @param address Host address; may be an IP address in string form (e.g., "43.195.83.32"), or it may be a host name (e.g., "example.com")
 * @param port Host port, in native byte order
 *
 * The socket first enters @c HostLookupState, then performs a host name lookup of @a address (only if required). If the lookup succeeds, @c hostFound() is emitted
 * and SocketConnector enters @c ConnectingState. It then attempts to connect to the address or addresses returned by the lookup.
 * Finally, if a connection is established, SocketConnector enters @c ConnectedState and emits @c connected().
 *
 * At any point, the socket can emit error() to signal that an error occurred.
 */
void SocketConnector::connectToHost(const QString& address, quint16 port)
{
	Q_D(SocketConnector);
	d->connectToHost(address, port);
}

/**
 * @brief Attempts to make a connection to @a address on the given @a port.
 * @param address Host address; may be an IP address in string form (e.g., "43.195.83.32"), or it may be a host name (e.g., "example.com")
 * @param port Host port, in native byte order
 * @overload
 */
void SocketConnector::connectToHost(const QHostAddress& address, quint16 port)
{
	this->connectToHost(address.toString(), port);
}

/**
 * @brief Attempts to close the socket.
 *
 * SocketConnector first enters @c ClosingState, close the socket and then enters @c UnconnectedState.
 * If the previous state of the socket has been @c ConnectedState, the @c disconnected() signal is enitted.
 */
void SocketConnector::disconnectFromHost(void)
{
	Q_D(SocketConnector);
	d->disconnectFromHost();
}

/**
 * @brief Aborts the current connection to the host.
 */
void SocketConnector::abort(void)
{
	Q_D(SocketConnector);
	d->abort();
}

/**
 * @brief Waits until the socket is connected, up to @a timeout milliseconds.
 * @param timeout
 * @return Whether the socket was successfully connected to the target.
 */
bool SocketConnector::waitForConnected(int timeout)
{
	Q_D(SocketConnector);
	return d->waitForConnected(timeout);
}

/**
 * @brief Assigns the connected socket to a @a target
 * @param target
 * @return Whether a call to @c target->setSocketDescriptor() succeeded
 */
bool SocketConnector::assignTo(QAbstractSocket* target)
{
	Q_D(SocketConnector);
	int fd = d->m_fd;

	if (QAbstractSocket::ConnectedState == d->m_state) {
		bool res = target->setSocketDescriptor(fd, QAbstractSocket::ConnectedState, QIODevice::ReadWrite);
		if (res) {
			d->m_fd    = -1;
			d->m_state = QAbstractSocket::UnconnectedState;
		}

		return true;
	}

	return false;
}

/**
 * @brief Returns the socket type (TCP, UDP, or other).
 * @return Socket type
 */
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

/**
 * @brief Returns the state of the socket.
 * @return Socket state
 */
QAbstractSocket::SocketState SocketConnector::state(void) const
{
	Q_D(const SocketConnector);
	return d->m_state;
}

/**
 * @brief Returns the type of error that last occurred.
 * @return Socket error
 */
QAbstractSocket::SocketError SocketConnector::error(void) const
{
	Q_D(const SocketConnector);
	return d->m_error;
}

/**
 * @brief Returns the native socket descriptor if this is available; otherwise returns -1.
 * @return Native socket descriptor
 */
qintptr SocketConnector::socketDescriptor(void) const
{
	Q_D(const SocketConnector);
	return d->m_fd;
}

#include "moc_socketconnector.cpp"
