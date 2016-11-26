/**
 * Copyright 2015 Vineet Garg <albertvaka@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../core/backends/lan/server.h"
#include "../core/backends/lan/socketlinereader.h"

#include <QSslKey>
#include <QtCrypto>
#include <QTest>
#include <QTimer>

/*
 * This class tests the behaviour of socket line reader when the connection if over ssl. Since SSL takes part below application layer,
 * working of SocketLineReader should be same.
 */
class TestSslSocketLineReader : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void newPackage();

private Q_SLOTS:

    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

    void testTrustedDevice();
    void testUntrustedDevice();
    void testTrustedDeviceWithWrongCertificate();


private:
    const int PORT = 7894;
    QTimer mTimer;
    QCA::Initializer mQcaInitializer;
    QEventLoop mLoop;
    QList<QByteArray> mPackages;
    Server *mServer;
    QSslSocket *mClientSocket;
    SocketLineReader *mReader;

private:
    void setSocketAttributes(QSslSocket* socket, QString deviceName);
};

void TestSslSocketLineReader::initTestCase()
{
    mServer = new Server(this);

    QVERIFY2(mServer->listen(QHostAddress::LocalHost, PORT), "Failed to create local tcp server");

    mTimer.setInterval(10 * 1000);//Ten second is more enough for this test, just used this so that to break mLoop if stuck
    mTimer.setSingleShot(true);
    connect(&mTimer, &QTimer::timeout, &mLoop, &QEventLoop::quit);

    mTimer.start();
}

void TestSslSocketLineReader::init()
{
    mClientSocket = new QSslSocket(this);
    mClientSocket->connectToHost(QHostAddress::LocalHost, PORT);
    connect(mClientSocket, &QAbstractSocket::connected, &mLoop, &QEventLoop::quit);

    mLoop.exec();

    QVERIFY2(mClientSocket->isOpen(), "Could not connect to local tcp server");
}

void TestSslSocketLineReader::cleanup()
{
    mClientSocket->disconnectFromHost();
    delete mClientSocket;
}

void TestSslSocketLineReader::cleanupTestCase()
{
    delete mServer;
}

void TestSslSocketLineReader::testTrustedDevice()
{

    int maxAttemps = 5;
    QCOMPARE(true, mServer->hasPendingConnections());
    while(!mServer->hasPendingConnections() && maxAttemps > 0) {
        --maxAttemps;
        QTest::qSleep(1000);
    }

    QSslSocket *serverSocket = mServer->nextPendingConnection();

    QVERIFY2(serverSocket != 0, "Null socket returned by server");
    QVERIFY2(serverSocket->isOpen(), "Server socket already closed");

    setSocketAttributes(serverSocket, QStringLiteral("Test Server"));
    setSocketAttributes(mClientSocket, QStringLiteral("Test Client"));

    serverSocket->setPeerVerifyName(QStringLiteral("Test Client"));
    serverSocket->setPeerVerifyMode(QSslSocket::VerifyPeer);
    serverSocket->addCaCertificate(mClientSocket->localCertificate());

    mClientSocket->setPeerVerifyName(QStringLiteral("Test Server"));
    mClientSocket->setPeerVerifyMode(QSslSocket::VerifyPeer);
    mClientSocket->addCaCertificate(serverSocket->localCertificate());

    connect(mClientSocket, &QSslSocket::encrypted, &mLoop, &QEventLoop::quit);
    serverSocket->startServerEncryption();
    mClientSocket->startClientEncryption();
    mLoop.exec();

    // Both client and server socket should be encrypted here and should have remote certificate because VerifyPeer is used
    QVERIFY2(mClientSocket->isOpen(), "Client socket already closed");
    QVERIFY2(serverSocket->isOpen(), "Server socket already closed");
    QVERIFY2(mClientSocket->isEncrypted(), "Client is not encrypted");
    QVERIFY2(serverSocket->isEncrypted(), "Server is not encrypted");
    QVERIFY2(!mClientSocket->peerCertificate().isNull(), "Server certificate not received");
    QVERIFY2(!serverSocket->peerCertificate().isNull(), "Client certificate not received");

    QList<QByteArray> dataToSend;
    dataToSend << "foobar\n" << "barfoo\n" << "foobar?\n" << "\n" << "barfoo!\n" << "panda\n";
    Q_FOREACH(const QByteArray &line, dataToSend) {
        mClientSocket->write(line);
    }
    mClientSocket->flush();

    mPackages.clear();

    mReader = new SocketLineReader(serverSocket, this);
    connect(mReader, &SocketLineReader::readyRead, this,&TestSslSocketLineReader::newPackage);
    mLoop.exec();

    /* remove the empty line before compare */
    dataToSend.removeOne("\n");

    QCOMPARE(mPackages.count(), 5);//We expect 5 Packages
    for(int x = 0;x < 5; ++x) {
        QCOMPARE(mPackages[x], dataToSend[x]);
    }

    delete mReader;
}

void TestSslSocketLineReader::testUntrustedDevice()
{
    int maxAttemps = 5;
    QCOMPARE(true, mServer->hasPendingConnections());
    while(!mServer->hasPendingConnections() && maxAttemps > 0) {
        --maxAttemps;
        QTest::qSleep(1000);
    }

    QSslSocket *serverSocket = mServer->nextPendingConnection();

    QVERIFY2(serverSocket != 0, "Null socket returned by server");
    QVERIFY2(serverSocket->isOpen(), "Server socket already closed");

    setSocketAttributes(serverSocket, QStringLiteral("Test Server"));
    setSocketAttributes(mClientSocket, QStringLiteral("Test Client"));

    serverSocket->setPeerVerifyName(QStringLiteral("Test Client"));
    serverSocket->setPeerVerifyMode(QSslSocket::QueryPeer);

    mClientSocket->setPeerVerifyName(QStringLiteral("Test Server"));
    mClientSocket->setPeerVerifyMode(QSslSocket::QueryPeer);

    connect(mClientSocket, &QSslSocket::encrypted, &mLoop, &QEventLoop::quit);
    serverSocket->startServerEncryption();
    mClientSocket->startClientEncryption();
    mLoop.exec();

    QVERIFY2(mClientSocket->isOpen(), "Client socket already closed");
    QVERIFY2(serverSocket->isOpen(), "Server socket already closed");
    QVERIFY2(mClientSocket->isEncrypted(), "Client is not encrypted");
    QVERIFY2(serverSocket->isEncrypted(), "Server is not encrypted");
    QVERIFY2(!mClientSocket->peerCertificate().isNull(), "Server certificate not received");
    QVERIFY2(!serverSocket->peerCertificate().isNull(), "Client certificate not received");

    QList<QByteArray> dataToSend;
    dataToSend << "foobar\n" << "barfoo\n" << "foobar?\n" << "\n" << "barfoo!\n" << "panda\n";
    Q_FOREACH(const QByteArray &line, dataToSend) {
            mClientSocket->write(line);
        }
    mClientSocket->flush();

    mPackages.clear();

    mReader = new SocketLineReader(serverSocket, this);
    connect(mReader, &SocketLineReader::readyRead, this, &TestSslSocketLineReader::newPackage);
    mLoop.exec();

    /* remove the empty line before compare */
    dataToSend.removeOne("\n");

    QCOMPARE(mPackages.count(), 5);//We expect 5 Packages
    for(int x = 0;x < 5; ++x) {
        QCOMPARE(mPackages[x], dataToSend[x]);
    }

    delete mReader;
}

void TestSslSocketLineReader::testTrustedDeviceWithWrongCertificate()
{
    int maxAttemps = 5;
    while(!mServer->hasPendingConnections() && maxAttemps > 0) {
        --maxAttemps;
        QTest::qSleep(1000);
    }

    QSslSocket *serverSocket = mServer->nextPendingConnection();

    QVERIFY2(serverSocket != 0, "Could not open a connection to the client");

    setSocketAttributes(serverSocket, QStringLiteral("Test Server"));
    setSocketAttributes(mClientSocket, QStringLiteral("Test Client"));

    // Not adding other device certificate to list of CA certificate, and using VerifyPeer. This should lead to handshake failure
    serverSocket->setPeerVerifyName(QStringLiteral("Test Client"));
    serverSocket->setPeerVerifyMode(QSslSocket::VerifyPeer);

    mClientSocket->setPeerVerifyName(QStringLiteral("Test Server"));
    mClientSocket->setPeerVerifyMode(QSslSocket::VerifyPeer);

    connect(serverSocket, &QSslSocket::encrypted, &mLoop, &QEventLoop::quit); // Encrypted signal should never be emitted
    connect(mClientSocket, &QSslSocket::encrypted, &mLoop, &QEventLoop::quit); // Encrypted signal should never be emitted
    connect(serverSocket, &QAbstractSocket::disconnected, &mLoop, &QEventLoop::quit);
    connect(mClientSocket, &QAbstractSocket::disconnected, &mLoop, &QEventLoop::quit);

    serverSocket->startServerEncryption();
    mClientSocket->startClientEncryption();
    mLoop.exec();

    QVERIFY2(!serverSocket->isEncrypted(), "Server is encrypted, it should not");
    QVERIFY2(!mClientSocket->isEncrypted(), "lient is encrypted, it should now");

    if (serverSocket->state() != QAbstractSocket::UnconnectedState) mLoop.exec(); // Wait until serverSocket is disconnected, It should be in disconnected state
    if (mClientSocket->state() != QAbstractSocket::UnconnectedState) mLoop.exec(); // Wait until mClientSocket is disconnected, It should be in disconnected state

    QCOMPARE((int)mClientSocket->state(), 0);
    QCOMPARE((int)serverSocket->state(), 0);

}

void TestSslSocketLineReader::newPackage()
{
    if (!mReader->bytesAvailable()) {
        return;
    }

    int maxLoops = 5;
    while(mReader->bytesAvailable() > 0 && maxLoops > 0) {
        --maxLoops;
        const QByteArray package = mReader->readLine();
        if (!package.isEmpty()) {
            mPackages.append(package);
        }

        if (mPackages.count() == 5) {
            mLoop.exit();
        }
    }
}

void TestSslSocketLineReader::setSocketAttributes(QSslSocket *socket, QString deviceName) {

    QDateTime startTime = QDateTime::currentDateTime();
    QDateTime endTime = startTime.addYears(10);
    QCA::CertificateInfo certificateInfo;
    certificateInfo.insert(QCA::CommonName,deviceName);
    certificateInfo.insert(QCA::Organization,QStringLiteral("KDE"));
    certificateInfo.insert(QCA::OrganizationalUnit,QStringLiteral("Kde connect"));

    QCA::CertificateOptions certificateOptions(QCA::PKCS10);
    certificateOptions.setSerialNumber(10);
    certificateOptions.setInfo(certificateInfo);
    certificateOptions.setValidityPeriod(startTime, endTime);
    certificateOptions.setFormat(QCA::PKCS10);

    QCA::PrivateKey privKey = QCA::KeyGenerator().createRSA(2048);
    QSslCertificate certificate = QSslCertificate(QCA::Certificate(certificateOptions, privKey).toPEM().toLatin1());

    socket->setPrivateKey(QSslKey(privKey.toPEM().toLatin1(), QSsl::Rsa));
    socket->setLocalCertificate(certificate);

}

QTEST_GUILESS_MAIN(TestSslSocketLineReader)

#include "testsslsocketlinereader.moc"

