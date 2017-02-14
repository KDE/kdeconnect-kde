/**
 * Copyright 2015 Vineet Garg <grgvineet@gmail.com>
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

// This class tests the behaviour of the class LanLinkProvider, be sure to kill process kdeconnectd to avoid any port binding issues


#include "../core/backends/lan/lanlinkprovider.h"
#include "../core/backends/lan/server.h"
#include "../core/backends/lan/socketlinereader.h"
#include "../core/kdeconnectconfig.h"

#include <QAbstractSocket>
#include <QSslSocket>
#include <QtTest>
#include <QSslKey>
#include <QUdpSocket>

/*
 * This class tests the working of LanLinkProvider under different conditions that when identity package is received over TCP, over UDP and same when the device is paired.
 * It depends on KdeConnectConfig since LanLinkProvider internally uses it.
 */
class LanLinkProviderTest : public QObject
{
    Q_OBJECT
public:
    explicit LanLinkProviderTest()
    : mLanLinkProvider(true) {
        QStandardPaths::setTestModeEnabled(true);
    }

public Q_SLOTS:
    void initTestCase();

private Q_SLOTS:

    void pairedDeviceTcpPackageReceived();
    void pairedDeviceUdpPackageReceived();

    void unpairedDeviceTcpPackageReceived();
    void unpairedDeviceUdpPackageReceived();


private:
    const int TEST_PORT = 8520;
    // Add some private fields here
    LanLinkProvider mLanLinkProvider;
    Server* mServer;
    SocketLineReader* mReader;
    QUdpSocket* mUdpSocket;
    QString mIdentityPackage;

    // Attributes for test device
    QString deviceId;
    QString name;
    QCA::PrivateKey privateKey;
    QSslCertificate certificate;

    QSslCertificate generateCertificate(QString&, QCA::PrivateKey&);
    void addTrustedDevice();
    void removeTrustedDevice();
    void setSocketAttributes(QSslSocket *socket);
    void testIdentityPackage(QByteArray& identityPackage);

};

void LanLinkProviderTest::initTestCase()
{
    removeTrustedDevice(); // Remove trusted device if left by chance by any test

    deviceId = QStringLiteral("testdevice");
    name = QStringLiteral("Test Device");
    privateKey = QCA::KeyGenerator().createRSA(2048);
    certificate = generateCertificate(deviceId, privateKey);

    mLanLinkProvider.onStart();

    mIdentityPackage = QStringLiteral("{\"id\":1439365924847,\"type\":\"kdeconnect.identity\",\"body\":{\"deviceId\":\"testdevice\",\"deviceName\":\"Test Device\",\"protocolVersion\":6,\"deviceType\":\"phone\",\"tcpPort\":") + QString::number(TEST_PORT) + QStringLiteral("}}");
}

void LanLinkProviderTest::pairedDeviceTcpPackageReceived()
{
    KdeConnectConfig* kcc = KdeConnectConfig::instance();
    addTrustedDevice();

    QUdpSocket* mUdpServer = new QUdpSocket;
    bool b = mUdpServer->bind(QHostAddress::LocalHost, LanLinkProvider::UDP_PORT, QUdpSocket::ShareAddress);
    QVERIFY(b);

    QSignalSpy spy(mUdpServer, SIGNAL(readyRead()));
    mLanLinkProvider.onNetworkChange();
    QVERIFY(!spy.isEmpty() || spy.wait());

    QByteArray datagram;
    datagram.resize(mUdpServer->pendingDatagramSize());
    QHostAddress sender;

    mUdpServer->readDatagram(datagram.data(), datagram.size(), &sender);

    testIdentityPackage(datagram);

    QJsonDocument jsonDocument = QJsonDocument::fromJson(datagram);
    QJsonObject body = jsonDocument.object().value(QStringLiteral("body")).toObject();
    int tcpPort = body.value(QStringLiteral("tcpPort")).toInt();

    QSslSocket socket;
    QSignalSpy spy2(&socket, SIGNAL(connected()));

    socket.connectToHost(sender, tcpPort);
    QVERIFY(spy2.wait());

    QVERIFY2(socket.isOpen(), "Socket disconnected immediately");

    socket.write(mIdentityPackage.toLatin1());
    socket.waitForBytesWritten(2000);

    QSignalSpy spy3(&socket, SIGNAL(encrypted()));

    setSocketAttributes(&socket);
    socket.addCaCertificate(kcc->certificate());
    socket.setPeerVerifyMode(QSslSocket::VerifyPeer);
    socket.setPeerVerifyName(kcc->name());

    socket.startServerEncryption();
    QVERIFY(spy3.wait());

    QCOMPARE(socket.sslErrors().size(), 0);
    QVERIFY2(socket.isValid(), "Server socket disconnected");
    QVERIFY2(socket.isEncrypted(), "Server socket not yet encrypted");
    QVERIFY2(!socket.peerCertificate().isNull(), "Peer certificate is null");

    removeTrustedDevice();
    delete mUdpServer;
}

void LanLinkProviderTest::pairedDeviceUdpPackageReceived()
{
    KdeConnectConfig* kcc = KdeConnectConfig::instance();
    addTrustedDevice();

    mServer = new Server(this);
    mUdpSocket = new QUdpSocket(this);

    mServer->listen(QHostAddress::LocalHost, TEST_PORT);

    QSignalSpy spy(mServer, SIGNAL(newConnection()));

    qint64 bytesWritten = mUdpSocket->writeDatagram(mIdentityPackage.toLatin1(), QHostAddress::LocalHost, LanLinkProvider::UDP_PORT); // write an identity package to udp socket here, we do not broadcast it here
    QCOMPARE(bytesWritten, mIdentityPackage.size());

    // We should have an incoming connection now, wait for incoming connection
    QVERIFY(!spy.isEmpty() || spy.wait());

    QSslSocket* serverSocket = mServer->nextPendingConnection();

    QVERIFY2(serverSocket != 0, "Server socket is null");
    QVERIFY2(serverSocket->isOpen(), "Server socket already closed");

    mReader = new SocketLineReader(serverSocket, this);
    QSignalSpy spy2(mReader, SIGNAL(readyRead()));
    QVERIFY(spy2.wait());

    QByteArray receivedPackage = mReader->readLine();
    testIdentityPackage(receivedPackage);
    // Received identiy package from LanLinkProvider now start ssl

    QSignalSpy spy3(serverSocket, SIGNAL(encrypted()));
    QVERIFY(connect(serverSocket, static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QSslSocket::error),
            this, [](QAbstractSocket::SocketError error){ qDebug() << "error:" << error; }));

    setSocketAttributes(serverSocket);
    serverSocket->addCaCertificate(kcc->certificate());
    serverSocket->setPeerVerifyMode(QSslSocket::VerifyPeer);
    serverSocket->setPeerVerifyName(kcc->deviceId());

    serverSocket->startClientEncryption(); // Its TCP server. but SSL client
    QVERIFY(!serverSocket->isEncrypted());
    spy3.wait(2000);
    qDebug() << "xxxxxxxxx" << serverSocket->sslErrors();

    QCOMPARE(serverSocket->sslErrors().size(), 0);
    QVERIFY2(serverSocket->isValid(), "Server socket disconnected");
    QVERIFY2(serverSocket->isEncrypted(), "Server socket not yet encrypted");
    QVERIFY2(!serverSocket->peerCertificate().isNull(), "Peer certificate is null");

    removeTrustedDevice();

    delete mServer;
    delete mUdpSocket;
}

void LanLinkProviderTest::unpairedDeviceTcpPackageReceived()
{
    QUdpSocket* mUdpServer = new QUdpSocket;
    bool b = mUdpServer->bind(QHostAddress::LocalHost, LanLinkProvider::UDP_PORT, QUdpSocket::ShareAddress);
    QVERIFY(b);

    QSignalSpy spy(mUdpServer, SIGNAL(readyRead()));
    mLanLinkProvider.onNetworkChange();
    QVERIFY(!spy.isEmpty() || spy.wait());

    QByteArray datagram;
    datagram.resize(mUdpServer->pendingDatagramSize());
    QHostAddress sender;

    mUdpServer->readDatagram(datagram.data(), datagram.size(), &sender);

    testIdentityPackage(datagram);

    QJsonDocument jsonDocument = QJsonDocument::fromJson(datagram);
    QJsonObject body = jsonDocument.object().value(QStringLiteral("body")).toObject();
    int tcpPort = body.value(QStringLiteral("tcpPort")).toInt();

    QSslSocket socket;
    QSignalSpy spy2(&socket, SIGNAL(connected()));

    socket.connectToHost(sender, tcpPort);
    QVERIFY(spy2.wait());

    QVERIFY2(socket.isOpen(), "Socket disconnected immediately");

    socket.write(mIdentityPackage.toLatin1());
    socket.waitForBytesWritten(2000);

    QSignalSpy spy3(&socket, SIGNAL(encrypted()));
    // We don't take care for sslErrors signal here, but signal will emit still we will get successful connection

    setSocketAttributes(&socket);
    socket.setPeerVerifyMode(QSslSocket::QueryPeer);

    socket.startServerEncryption();

    QVERIFY(spy3.wait());

    QVERIFY2(socket.isValid(), "Server socket disconnected");
    QVERIFY2(socket.isEncrypted(), "Server socket not yet encrypted");
    QVERIFY2(!socket.peerCertificate().isNull(), "Peer certificate is null");

    delete mUdpServer;
}

void LanLinkProviderTest::unpairedDeviceUdpPackageReceived()
{
    mServer = new Server(this);
    mUdpSocket = new QUdpSocket(this);

    mServer->listen(QHostAddress::LocalHost, TEST_PORT);

    QSignalSpy spy(mServer, &Server::newConnection);
    qint64 bytesWritten = mUdpSocket->writeDatagram(mIdentityPackage.toLatin1(), QHostAddress::LocalHost, LanLinkProvider::UDP_PORT); // write an identity package to udp socket here, we do not broadcast it here
    QCOMPARE(bytesWritten, mIdentityPackage.size());

    QVERIFY(!spy.isEmpty() || spy.wait());

    QSslSocket* serverSocket = mServer->nextPendingConnection();

    QVERIFY2(serverSocket != 0, "Server socket is null");
    QVERIFY2(serverSocket->isOpen(), "Server socket already closed");

    mReader = new SocketLineReader(serverSocket, this);
    QSignalSpy spy2(mReader, &SocketLineReader::readyRead);
    QVERIFY(spy2.wait());

    QByteArray receivedPackage = mReader->readLine();
    QVERIFY2(!receivedPackage.isEmpty(), "Empty package received");

    testIdentityPackage(receivedPackage);

    // Received identity package from LanLinkProvider now start ssl

    QSignalSpy spy3(serverSocket, SIGNAL(encrypted()));

    setSocketAttributes(serverSocket);
    serverSocket->setPeerVerifyMode(QSslSocket::QueryPeer);
    serverSocket->startClientEncryption(); // Its TCP server. but SSL client
    QVERIFY(spy3.wait());

    QVERIFY2(serverSocket->isValid(), "Server socket disconnected");
    QVERIFY2(serverSocket->isEncrypted(), "Server socket not yet encrypted");
    QVERIFY2(!serverSocket->peerCertificate().isNull(), "Peer certificate is null");

    delete mServer;
    delete mUdpSocket;
}

void LanLinkProviderTest::testIdentityPackage(QByteArray &identityPackage)
{
    QJsonDocument jsonDocument = QJsonDocument::fromJson(identityPackage);
    QJsonObject jsonObject = jsonDocument.object();
    QJsonObject body = jsonObject.value(QStringLiteral("body")).toObject();

    QCOMPARE(jsonObject.value("type").toString(), QString("kdeconnect.identity"));
    QVERIFY2(body.contains("deviceName"), "Device name not found in identity package");
    QVERIFY2(body.contains("deviceId"), "Device id not found in identity package");
    QVERIFY2(body.contains("protocolVersion"), "Protocol version not found in identity package");
    QVERIFY2(body.contains("deviceType"), "Device type not found in identity package");
}

QSslCertificate LanLinkProviderTest::generateCertificate(QString& commonName, QCA::PrivateKey& privateKey)
{
    QDateTime startTime = QDateTime::currentDateTime();
    QDateTime endTime = startTime.addYears(10);
    QCA::CertificateInfo certificateInfo;
    certificateInfo.insert(QCA::CommonName,commonName);
    certificateInfo.insert(QCA::Organization,QStringLiteral("KDE"));
    certificateInfo.insert(QCA::OrganizationalUnit,QStringLiteral("Kde connect"));

    QCA::CertificateOptions certificateOptions(QCA::PKCS10);
    certificateOptions.setSerialNumber(10);
    certificateOptions.setInfo(certificateInfo);
    certificateOptions.setValidityPeriod(startTime, endTime);
    certificateOptions.setFormat(QCA::PKCS10);

    QSslCertificate certificate = QSslCertificate(QCA::Certificate(certificateOptions, privateKey).toPEM().toLatin1());
    return certificate;
}

void LanLinkProviderTest::setSocketAttributes(QSslSocket *socket)
{
    socket->setPrivateKey(QSslKey(privateKey.toPEM().toLatin1(), QSsl::Rsa));
    socket->setLocalCertificate(certificate);
}

void LanLinkProviderTest::addTrustedDevice()
{
    KdeConnectConfig *kcc = KdeConnectConfig::instance();
    kcc->addTrustedDevice(deviceId, name, QStringLiteral("phone"));
    kcc->setDeviceProperty(deviceId, QStringLiteral("certificate"), QString::fromLatin1(certificate.toPem()));
}

void LanLinkProviderTest::removeTrustedDevice()
{
    KdeConnectConfig *kcc = KdeConnectConfig::instance();
    kcc->removeTrustedDevice(deviceId);
}


QTEST_GUILESS_MAIN(LanLinkProviderTest)

#include "lanlinkprovidertest.moc"
