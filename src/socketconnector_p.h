#ifndef SOCKETCONNECTOR_P_H
#define SOCKETCONNECTOR_P_H

#include <QtNetwork/QHostAddress>
#include <QtNetwork/QHostInfo>

QT_FORWARD_DECLARE_CLASS(QSocketNotifier)
QT_FORWARD_DECLARE_CLASS(QTimer)

class SocketConnector;

class SocketConnectorPrivate {
	Q_DECLARE_PUBLIC(SocketConnector)
	SocketConnector* const q_ptr;
public:
	SocketConnectorPrivate(SocketConnector* const q);
	~SocketConnectorPrivate(void);

	bool createSocket(int domain, int type, int proto);
	bool bindTo(const QHostAddress& a, quint16 port);
	void connectToHost(const QString& address, quint16 port);
	void disconnectFromHost(void);
	void abort(void);

	bool waitForConnected(int timeout);
private:
	int m_fd;
	int m_domain;
	int m_type;
	int m_proto;
	int m_port;
	QAbstractSocket::SocketState m_state;
	QAbstractSocket::SocketError m_error;
	QList<QHostAddress> m_addresses;
	QHostAddress m_bound_address;
	quint16 m_bound_port;
	int m_lookup_id;
	QSocketNotifier* m_notifier;
	QTimer* m_timer;

	int recreateSocket(void);
	bool bindV4(const QHostAddress& a, quint16 port);
	bool bindV6(const QHostAddress& a, quint16 port);
	int connectV4(const QHostAddress& a);
	int connectV6(const QHostAddress& a);

	void _q_startConnecting(const QHostInfo& info);
	void _q_connectToNextAddress(void);
	void _q_connected(int sock);
	void _q_abortConnection(void);
};

#endif // SOCKETCONNECTOR_P_H
