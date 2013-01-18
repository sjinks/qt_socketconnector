#ifndef SOCKETCONNECTOR_H
#define SOCKETCONNECTOR_H

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QHostInfo>

class SocketConnectorPrivate;

class SocketConnector : public QObject {
	Q_OBJECT
public:
	SocketConnector(QObject* parent = 0);
	virtual ~SocketConnector(void);
	bool createSocket(int domain, int type, int proto = 0);
	bool createTcpSocket(void);
	bool createUdpSocket(void);
	bool bindTo(const QHostAddress& a, quint16 port = 0);
	void connectToHost(const QString& address, quint16 port);
	void connectToHost(const QHostAddress& address, quint16 port);
	void disconnectFromHost(void);
	void abort(void);

	bool waitForConnected(int timeout = 30000);

	bool assignTo(QAbstractSocket* target);

	QAbstractSocket::SocketType socketType(void) const;
	QAbstractSocket::SocketState state(void) const;
	QAbstractSocket::SocketError error(void) const;

	qintptr socketDescriptor(void) const;

Q_SIGNALS:
	void hostFound(void);
	void connected(void);
	void disconnected(void);
	void stateChanged(QAbstractSocket::SocketState);
	void error(QAbstractSocket::SocketError);

private:
	Q_DISABLE_COPY(SocketConnector)
	Q_DECLARE_PRIVATE(SocketConnector)
	QScopedPointer<SocketConnectorPrivate> d_ptr;

	Q_PRIVATE_SLOT(d_func(), void _q_startConnecting(QHostInfo))
	Q_PRIVATE_SLOT(d_func(), void _q_connectToNextAddress())
	Q_PRIVATE_SLOT(d_func(), void _q_connected(int))
	Q_PRIVATE_SLOT(d_func(), void _q_abortConnection())

};

#endif // SOCKETCONNECTOR_H
