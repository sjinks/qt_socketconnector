#ifndef SOCKETCONNECTOR_H
#define SOCKETCONNECTOR_H

#include <QtCore/QObject>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QHostInfo>

QT_FORWARD_DECLARE_CLASS(QSocketNotifier)
QT_FORWARD_DECLARE_CLASS(QTimer)

class SocketConnector : public QObject {
	Q_OBJECT
public:
	SocketConnector(QObject* parent = 0);
	virtual ~SocketConnector(void);
	bool createSocket(int domain, int type, int proto = 0);
	bool bindTo(const QHostAddress& a, quint16 port = 0);
	void connectToHost(const QString& address, quint16 port);
	void connectToHost(const QHostAddress& address, quint16 port);
	void disconnectFromHost(void);

	bool assignTo(QAbstractSocket* target);

Q_SIGNALS:
	void hostFound(void);
	void connected(void);
	void disconnected(void);
	void stateChanged(QAbstractSocket::SocketState);
	void error(QAbstractSocket::SocketError);

public Q_SLOTS:

private Q_SLOTS:
	void _q_startConnecting(const QHostInfo& info);
	void _q_connectToNextAddress(void);
	void _q_connected(int sock);
	void _q_abortConnection(void);

private:
	int m_fd;
	int m_domain;
	int m_type;
	int m_proto;
	int m_port;
	QAbstractSocket::SocketState m_state;
	QList<QHostAddress> m_addresses;
	QHostAddress m_bound_address;
	quint16 m_bound_port;
	QSocketNotifier* m_notifier;
	QTimer* m_timer;

	int recreateSocket(void);
	bool bindV4(const QHostAddress& a, quint16 port);
	bool bindV6(const QHostAddress& a, quint16 port);
	int connectV4(const QHostAddress& a);
	int connectV6(const QHostAddress& a);
};

#endif // SOCKETCONNECTOR_H
