#include <QtCore/QCoreApplication>
#include <QtCore/QEventLoop>
#include <QtCore/QTimer>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QNetworkAddressEntry>
#include <QtNetwork/QNetworkInterface>
#include <QtTest/QTest>
#include "socketconnector.h"

class SocketConnectorTest : public QObject {
	Q_OBJECT
public:
	explicit SocketConnectorTest(QObject* parent = 0)
		: QObject(parent), m_addr(), m_conn(0), m_server(0), m_peer(), m_peer_port()
	{
	}

protected Q_SLOTS:
	void handleNewConnection(void)
	{
		QTcpServer* server = qobject_cast<QTcpServer*>(this->sender());
		Q_ASSERT(server != 0);

		QTcpSocket* sock = server->nextPendingConnection();
		Q_ASSERT(sock != 0);

		this->m_peer = sock->peerAddress();
		this->m_peer_port = sock->peerPort();
	}

private:
	QHostAddress m_addr;
	SocketConnector* m_conn;
	QTcpServer* m_server;
	QHostAddress m_peer;
	int m_peer_port;

private Q_SLOTS:
	void initTestCase(void)
	{
		bool found_lo = false;
		bool found_other = false;
		QNetworkInterface lo;
		QNetworkInterface other;
		QList<QNetworkAddressEntry> addrs;

		QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();
		for (int i=0; i<ifaces.size() && (!found_lo || !found_other); ++i) {
			const QNetworkInterface& iface = ifaces.at(i);
			QNetworkInterface::InterfaceFlags::Int flags = iface.flags().operator Int();

			if ((flags & QNetworkInterface::InterfaceFlags::Int(QNetworkInterface::IsUp | QNetworkInterface::IsRunning)) != 0 && iface.addressEntries().size() > 0) {
				if (flags & QNetworkInterface::InterfaceFlags::Int(QNetworkInterface::IsLoopBack)) {
					found_lo = true;
					lo = iface;
				}
				else {
					found_other = true;
					other = iface;
					addrs = iface.addressEntries();
				}
			}
		}

		if (!found_lo) {
			QSKIP("Failed to find a loopback interface");
		}

		if (!found_other) {
			QSKIP("Failed to find any other interface than loopback");
		}

		qDebug("Loopback interface: %s", qPrintable(lo.humanReadableName()));
		qDebug("Other interface: %s", qPrintable(other.humanReadableName()));

		for (int i=0; i<addrs.size(); ++i) {
			QHostAddress a = addrs.at(i).ip();
			if (a.protocol() == QAbstractSocket::IPv4Protocol) {
				this->m_addr = a;
				break;
			}
		}

		if (this->m_addr.isNull()) {
			this->m_addr = addrs.first().ip();
		}

		qDebug("IP Address for testing: %s", qPrintable(this->m_addr.toString()));
	}

	void init(void)
	{
		this->m_conn   = new SocketConnector(this);
		this->m_server = new QTcpServer(this);

		this->m_peer.clear();
		this->m_peer_port = -1;

		QObject::connect(this->m_server, SIGNAL(newConnection()), this, SLOT(handleNewConnection()));

		QVERIFY(this->m_server->listen(QHostAddress::LocalHost));
	}

	void cleanup(void)
	{
		delete this->m_server;
		delete this->m_conn;
		this->m_server = 0;
		this->m_conn   = 0;
	}

	void basicTest(void)
	{
		QCOMPARE(this->m_conn->socketType(), QAbstractSocket::UnknownSocketType);
		QCOMPARE(this->m_conn->state(), QAbstractSocket::UnconnectedState);

		QVERIFY(this->m_conn->createTcpSocket());
		QCOMPARE(this->m_conn->socketType(), QAbstractSocket::TcpSocket);
		QCOMPARE(this->m_conn->state(), QAbstractSocket::UnconnectedState);

		QVERIFY(this->m_conn->createUdpSocket());
		QCOMPARE(this->m_conn->socketType(), QAbstractSocket::UdpSocket);
		QCOMPARE(this->m_conn->state(), QAbstractSocket::UnconnectedState);

		QVERIFY(this->m_conn->createTcpSocket());
		QCOMPARE(this->m_conn->socketType(), QAbstractSocket::TcpSocket);
		QCOMPARE(this->m_conn->state(), QAbstractSocket::UnconnectedState);

		QVERIFY(this->m_conn->bindTo(this->m_addr));
		QCOMPARE(this->m_conn->socketType(), QAbstractSocket::TcpSocket);
		QCOMPARE(this->m_conn->state(), QAbstractSocket::BoundState);

		QVERIFY(!this->m_conn->bindTo(QHostAddress::LocalHost));
		QCOMPARE(this->m_conn->state(), QAbstractSocket::BoundState);
	}

	void testWaitForConnected(void)
	{
		this->m_conn->connectToHost(this->m_server->serverAddress(), this->m_server->serverPort());
		QVERIFY(!this->m_conn->waitForConnected());
		QCOMPARE(this->m_conn->state(), QAbstractSocket::UnconnectedState);

		QVERIFY(this->m_conn->createTcpSocket());
		QCOMPARE(this->m_conn->socketType(), QAbstractSocket::TcpSocket);
		QCOMPARE(this->m_conn->state(), QAbstractSocket::UnconnectedState);
		this->m_conn->connectToHost(this->m_server->serverAddress(), this->m_server->serverPort());
		QVERIFY(this->m_conn->waitForConnected());
		QCOMPARE(this->m_conn->state(), QAbstractSocket::ConnectedState);

		QVERIFY(!this->m_conn->createTcpSocket());
		QTcpSocket* s = new QTcpSocket(this);
		QVERIFY(this->m_conn->assignTo(s));
		QCOMPARE(this->m_conn->state(), QAbstractSocket::UnconnectedState);

		QVERIFY(this->m_conn->createTcpSocket());
		QCOMPARE(this->m_conn->state(), QAbstractSocket::UnconnectedState);
		QVERIFY(this->m_conn->bindTo(this->m_addr));
		QCOMPARE(this->m_conn->state(), QAbstractSocket::BoundState);

		this->m_conn->connectToHost(this->m_server->serverAddress(), this->m_server->serverPort());
		QVERIFY(this->m_conn->waitForConnected());
		QCOMPARE(this->m_conn->state(), QAbstractSocket::ConnectedState);
	}

	void testLoopback(void)
	{
		QVERIFY(this->m_conn->createTcpSocket());
		QCOMPARE(this->m_conn->socketType(), QAbstractSocket::TcpSocket);
		QCOMPARE(this->m_conn->state(), QAbstractSocket::UnconnectedState);

		QVERIFY(this->m_conn->bindTo(QHostAddress::LocalHost));
		QCOMPARE(this->m_conn->state(), QAbstractSocket::BoundState);

		QEventLoop loop;
		QVERIFY(QObject::connect(this->m_server, SIGNAL(newConnection()), &loop, SLOT(quit())));
		QTimer::singleShot(5000, &loop, SLOT(quit()));

		this->m_conn->connectToHost(this->m_server->serverAddress(), this->m_server->serverPort());
		loop.exec();

		QCOMPARE(this->m_conn->state(), QAbstractSocket::ConnectedState);
		QCOMPARE(this->m_peer, QHostAddress(QHostAddress::LocalHost));
	}

	void testLoopbackWithPort(void)
	{
		const quint16 port = 60001;

		QVERIFY(this->m_conn->createTcpSocket());
		QCOMPARE(this->m_conn->socketType(), QAbstractSocket::TcpSocket);
		QCOMPARE(this->m_conn->state(), QAbstractSocket::UnconnectedState);

		if (!this->m_conn->bindTo(QHostAddress::LocalHost, port)) {
			QSKIP("Skipping the test because bind() failed");
		}

		QCOMPARE(this->m_conn->state(), QAbstractSocket::BoundState);

		QEventLoop loop;
		QVERIFY(QObject::connect(this->m_server, SIGNAL(newConnection()), &loop, SLOT(quit())));
		QTimer::singleShot(5000, &loop, SLOT(quit()));

		this->m_conn->connectToHost(this->m_server->serverAddress(), this->m_server->serverPort());
		loop.exec();

		QCOMPARE(this->m_conn->state(), QAbstractSocket::ConnectedState);
		QCOMPARE(this->m_peer, QHostAddress(QHostAddress::LocalHost));
		QCOMPARE(this->m_peer_port, int(port));
	}

	void testLocalConnection(void)
	{
		QVERIFY(this->m_conn->createTcpSocket());
		QCOMPARE(this->m_conn->socketType(), QAbstractSocket::TcpSocket);
		QCOMPARE(this->m_conn->state(), QAbstractSocket::UnconnectedState);

		QEventLoop loop;
		QVERIFY(QObject::connect(this->m_server, SIGNAL(newConnection()), &loop, SLOT(quit())));
		QTimer::singleShot(5000, &loop, SLOT(quit()));

		this->m_conn->connectToHost(this->m_server->serverAddress(), this->m_server->serverPort());
		loop.exec();

		QCOMPARE(this->m_conn->state(), QAbstractSocket::ConnectedState);
		QCOMPARE(this->m_peer, QHostAddress(QHostAddress::LocalHost));
	}

	void testOtherInterface(void)
	{
		QVERIFY(this->m_conn->createTcpSocket());
		QCOMPARE(this->m_conn->socketType(), QAbstractSocket::TcpSocket);
		QCOMPARE(this->m_conn->state(), QAbstractSocket::UnconnectedState);

		QVERIFY(this->m_conn->bindTo(this->m_addr));
		QCOMPARE(this->m_conn->state(), QAbstractSocket::BoundState);

		QEventLoop loop;
		QVERIFY(QObject::connect(this->m_server, SIGNAL(newConnection()), &loop, SLOT(quit())));
		QTimer::singleShot(5000, &loop, SLOT(quit()));

		this->m_conn->connectToHost(this->m_server->serverAddress(), this->m_server->serverPort());
		loop.exec();

		QCOMPARE(this->m_conn->state(), QAbstractSocket::ConnectedState);
		QCOMPARE(this->m_peer, this->m_addr);
	}

	void testOtherInterfaceWithPort(void)
	{
		const quint16 port = 60001;

		QVERIFY(this->m_conn->createTcpSocket());
		QCOMPARE(this->m_conn->socketType(), QAbstractSocket::TcpSocket);
		QCOMPARE(this->m_conn->state(), QAbstractSocket::UnconnectedState);

		if (!this->m_conn->bindTo(this->m_addr, port)) {
			QSKIP("Skipping the test because bind() failed");
		}

		QCOMPARE(this->m_conn->state(), QAbstractSocket::BoundState);

		QEventLoop loop;
		QVERIFY(QObject::connect(this->m_server, SIGNAL(newConnection()), &loop, SLOT(quit())));
		QTimer::singleShot(5000, &loop, SLOT(quit()));

		this->m_conn->connectToHost(this->m_server->serverAddress(), this->m_server->serverPort());
		loop.exec();

		QCOMPARE(this->m_conn->state(), QAbstractSocket::ConnectedState);
		QCOMPARE(this->m_peer, this->m_addr);
		QCOMPARE(this->m_peer_port, int(port));
	}
};

int main(int argc, char** argv)
{
	QCoreApplication a(argc, argv);
	SocketConnectorTest t;
	return QTest::qExec(&t, argc, argv);
}

#include "tst_socketconnector.moc"
