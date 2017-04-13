/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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

#include "lanlinkprovider.h"
#include "core_debug.h"

#ifndef Q_OS_WIN
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#endif

#include <QHostInfo>
#include <QTcpServer>
#include <QNetworkProxy>
#include <QUdpSocket>
#include <QNetworkSession>
#include <QNetworkConfigurationManager>
#include <QSslCipher>
#include <QSslConfiguration>

#include "daemon.h"
#include "landevicelink.h"
#include "lanpairinghandler.h"
#include "kdeconnectconfig.h"

#define MIN_VERSION_WITH_SSL_SUPPORT 6

LanLinkProvider::LanLinkProvider(bool testMode)
    : mTestMode(testMode)
{
    mTcpPort = 0;

    combineBroadcastsTimer.setInterval(0); // increase this if waiting a single event-loop iteration is not enough
    combineBroadcastsTimer.setSingleShot(true);
    connect(&combineBroadcastsTimer, &QTimer::timeout, this, &LanLinkProvider::broadcastToNetwork);

    connect(&mUdpSocket, &QIODevice::readyRead, this, &LanLinkProvider::newUdpConnection);

    mServer = new Server(this);
    mServer->setProxy(QNetworkProxy::NoProxy);
    connect(mServer,&QTcpServer::newConnection,this, &LanLinkProvider::newConnection);

    mUdpSocket.setProxy(QNetworkProxy::NoProxy);

    //Detect when a network interface changes status, so we announce ourelves in the new network
    QNetworkConfigurationManager* networkManager = new QNetworkConfigurationManager(this);
    connect(networkManager, &QNetworkConfigurationManager::configurationChanged, this, &LanLinkProvider::onNetworkConfigurationChanged);

}

void LanLinkProvider::onNetworkConfigurationChanged(const QNetworkConfiguration &config)
{
    if (m_lastConfig != config && config.state() == QNetworkConfiguration::Active) {
        m_lastConfig = config;
        onNetworkChange();
    }
}

LanLinkProvider::~LanLinkProvider()
{
}

void LanLinkProvider::onStart()
{
    const QHostAddress bindAddress = mTestMode? QHostAddress::LocalHost : QHostAddress::Any;

    bool success = mUdpSocket.bind(bindAddress, UDP_PORT, QUdpSocket::ShareAddress);
    Q_ASSERT(success);

    qCDebug(KDECONNECT_CORE) << "onStart";

    mTcpPort = MIN_TCP_PORT;
    while (!mServer->listen(bindAddress, mTcpPort)) {
        mTcpPort++;
        if (mTcpPort > MAX_TCP_PORT) { //No ports available?
            qCritical(KDECONNECT_CORE) << "Error opening a port in range" << MIN_TCP_PORT << "-" << MAX_TCP_PORT;
            mTcpPort = 0;
            return;
        }
    }

    onNetworkChange();
}

void LanLinkProvider::onStop()
{
    qCDebug(KDECONNECT_CORE) << "onStop";
    mUdpSocket.close();
    mServer->close();
}

void LanLinkProvider::onNetworkChange()
{
    if (combineBroadcastsTimer.isActive()) {
        qCDebug(KDECONNECT_CORE()) << "Preventing duplicate broadcasts";
        return;
    }
    combineBroadcastsTimer.start();
}

//I'm in a new network, let's be polite and introduce myself
void LanLinkProvider::broadcastToNetwork()
{

    if (!mServer->isListening()) {
        //Not started
        return;
    }

    Q_ASSERT(mTcpPort != 0);

    qCDebug(KDECONNECT_CORE()) << "Broadcasting identity packet";

    QHostAddress destAddress = mTestMode? QHostAddress::LocalHost : QHostAddress(QStringLiteral("255.255.255.255"));

    NetworkPackage np(QLatin1String(""));
    NetworkPackage::createIdentityPackage(&np);
    np.set(QStringLiteral("tcpPort"), mTcpPort);

#ifdef Q_OS_WIN
    //On Windows we need to broadcast from every local IP address to reach all networks
    QUdpSocket sendSocket;
    sendSocket.setProxy(QNetworkProxy::NoProxy);
    for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
        if ( (iface.flags() & QNetworkInterface::IsUp)
          && (iface.flags() & QNetworkInterface::IsRunning)
          && (iface.flags() & QNetworkInterface::CanBroadcast)) {
            for (const QNetworkAddressEntry &ifaceAddress : iface.addressEntries()) {
                QHostAddress sourceAddress = ifaceAddress.ip();
                if (sourceAddress.protocol() == QAbstractSocket::IPv4Protocol && sourceAddress != QHostAddress::LocalHost) {
                    qCDebug(KDECONNECT_CORE()) << "Broadcasting as" << sourceAddress;
                    sendSocket.bind(sourceAddress, UDP_PORT);
                    sendSocket.writeDatagram(np.serialize(), destAddress, UDP_PORT);
                    sendSocket.close();
                }
            }
        }
    }
#else
    mUdpSocket.writeDatagram(np.serialize(), destAddress, UDP_PORT);
#endif

}

//I'm the existing device, a new device is kindly introducing itself.
//I will create a TcpSocket and try to connect. This can result in either connected() or connectError().
void LanLinkProvider::newUdpConnection() //udpBroadcastReceived
{
    while (mUdpSocket.hasPendingDatagrams()) {

        QByteArray datagram;
        datagram.resize(mUdpSocket.pendingDatagramSize());
        QHostAddress sender;

        mUdpSocket.readDatagram(datagram.data(), datagram.size(), &sender);

        if (sender.isLoopback() && !mTestMode)
            continue;

        NetworkPackage* receivedPackage = new NetworkPackage(QLatin1String(""));
        bool success = NetworkPackage::unserialize(datagram, receivedPackage);

        //qCDebug(KDECONNECT_CORE) << "udp connection from " << receivedPackage->;

        //qCDebug(KDECONNECT_CORE) << "Datagram " << datagram.data() ;

        if (!success || receivedPackage->type() != PACKAGE_TYPE_IDENTITY) {
            delete receivedPackage;
            continue;
        }

        if (receivedPackage->get<QString>(QStringLiteral("deviceId")) == KdeConnectConfig::instance()->deviceId()) {
            //qCDebug(KDECONNECT_CORE) << "Ignoring my own broadcast";
            delete receivedPackage;
            continue;
        }

        int tcpPort = receivedPackage->get<int>(QStringLiteral("tcpPort"));

        //qCDebug(KDECONNECT_CORE) << "Received Udp identity package from" << sender << " asking for a tcp connection on port " << tcpPort;

        QSslSocket* socket = new QSslSocket(this);
        socket->setProxy(QNetworkProxy::NoProxy);
        receivedIdentityPackages[socket].np = receivedPackage;
        receivedIdentityPackages[socket].sender = sender;
        connect(socket, &QAbstractSocket::connected, this, &LanLinkProvider::connected);
        connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(connectError()));
        socket->connectToHost(sender, tcpPort);
    }
}

void LanLinkProvider::connectError()
{
    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) return;
    disconnect(socket, &QAbstractSocket::connected, this, &LanLinkProvider::connected);
    disconnect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(connectError()));

    qCDebug(KDECONNECT_CORE) << "Fallback (1), try reverse connection (send udp packet)" << socket->errorString();
    NetworkPackage np(QLatin1String(""));
    NetworkPackage::createIdentityPackage(&np);
    np.set(QStringLiteral("tcpPort"), mTcpPort);
    mUdpSocket.writeDatagram(np.serialize(), receivedIdentityPackages[socket].sender, UDP_PORT);

    //The socket we created didn't work, and we didn't manage
    //to create a LanDeviceLink from it, deleting everything.
    delete receivedIdentityPackages.take(socket).np;
    delete socket;
}

//We received a UDP package and answered by connecting to them by TCP. This gets called on a succesful connection.
void LanLinkProvider::connected()
{
    qCDebug(KDECONNECT_CORE) << "Socket connected";

    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) return;
    disconnect(socket, &QAbstractSocket::connected, this, &LanLinkProvider::connected);
    disconnect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(connectError()));

    configureSocket(socket);

    // If socket disconnects due to any reason after connection, link on ssl faliure
    connect(socket, &QAbstractSocket::disconnected, socket, &QObject::deleteLater);

    NetworkPackage* receivedPackage = receivedIdentityPackages[socket].np;
    const QString& deviceId = receivedPackage->get<QString>(QStringLiteral("deviceId"));
    //qCDebug(KDECONNECT_CORE) << "Connected" << socket->isWritable();

    // If network is on ssl, do not believe when they are connected, believe when handshake is completed
    NetworkPackage np2(QLatin1String(""));
    NetworkPackage::createIdentityPackage(&np2);
    socket->write(np2.serialize());
    bool success = socket->waitForBytesWritten();

    if (success) {

        qCDebug(KDECONNECT_CORE) << "TCP connection done (i'm the existing device)";

        // if ssl supported
        if (receivedPackage->get<int>(QStringLiteral("protocolVersion")) >= MIN_VERSION_WITH_SSL_SUPPORT) {

            bool isDeviceTrusted = KdeConnectConfig::instance()->trustedDevices().contains(deviceId);
            configureSslSocket(socket, deviceId, isDeviceTrusted);

            qCDebug(KDECONNECT_CORE) << "Starting server ssl (I'm the client TCP socket)";

            connect(socket, &QSslSocket::encrypted, this, &LanLinkProvider::encrypted);

            if (isDeviceTrusted) {
                connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));
            }

            socket->startServerEncryption();

            return; // Return statement prevents from deleting received package, needed in slot "encrypted"
        } else {
            qWarning() << receivedPackage->get<QString>(QStringLiteral("deviceName")) << "uses an old protocol version, this won't work";
            //addLink(deviceId, socket, receivedPackage, LanDeviceLink::Remotely);
        }

    } else {
        //I think this will never happen, but if it happens the deviceLink
        //(or the socket that is now inside it) might not be valid. Delete them.
        qCDebug(KDECONNECT_CORE) << "Fallback (2), try reverse connection (send udp packet)";
        mUdpSocket.writeDatagram(np2.serialize(), receivedIdentityPackages[socket].sender, UDP_PORT);
    }

    delete receivedIdentityPackages.take(socket).np;
    //We don't delete the socket because now it's owned by the LanDeviceLink
}

void LanLinkProvider::encrypted()
{
    qCDebug(KDECONNECT_CORE) << "Socket succesfully stablished an SSL connection";

    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) return;
    disconnect(socket, &QSslSocket::encrypted, this, &LanLinkProvider::encrypted);
    disconnect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));

    Q_ASSERT(socket->mode() != QSslSocket::UnencryptedMode);
    LanDeviceLink::ConnectionStarted connectionOrigin = (socket->mode() == QSslSocket::SslClientMode)? LanDeviceLink::Locally : LanDeviceLink::Remotely;

    NetworkPackage* receivedPackage = receivedIdentityPackages[socket].np;
    const QString& deviceId = receivedPackage->get<QString>(QStringLiteral("deviceId"));

    addLink(deviceId, socket, receivedPackage, connectionOrigin);

    // Copied from connected slot, now delete received package
    delete receivedIdentityPackages.take(socket).np;
}

void LanLinkProvider::sslErrors(const QList<QSslError>& errors)
{
    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) return;

    disconnect(socket, &QSslSocket::encrypted, this, &LanLinkProvider::encrypted);
    disconnect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));

    Q_FOREACH (const QSslError &error, errors) {
        switch (error.error()) {
            case QSslError::CertificateSignatureFailed:
            case QSslError::CertificateNotYetValid:
            case QSslError::CertificateExpired:
            case QSslError::CertificateUntrusted:
            case QSslError::SelfSignedCertificate: {
                qCDebug(KDECONNECT_CORE) << "Failing due to " << error.errorString();
                // Due to simultaneous multiple connections, it may be possible that device instance does not exist anymore
                Device *device = Daemon::instance()->getDevice(socket->peerVerifyName());
                if (device != Q_NULLPTR) {
                    device->unpair();
                }
                break;
            }
            default:
                continue;
        }
    }

    delete receivedIdentityPackages.take(socket).np;
    // Socket disconnects itself on ssl error and will be deleted by deleteLater slot, no need to delete manually
}

//I'm the new device and this is the answer to my UDP identity package (no data received yet). They are connecting to us through TCP, and they should send an identity.
void LanLinkProvider::newConnection()
{
    //qCDebug(KDECONNECT_CORE) << "LanLinkProvider newConnection";

    while (mServer->hasPendingConnections()) {
        QSslSocket* socket = mServer->nextPendingConnection();
        configureSocket(socket);
        //This socket is still managed by us (and child of the QTcpServer), if
        //it disconnects before we manage to pass it to a LanDeviceLink, it's
        //our responsibility to delete it. We do so with this connection.
        connect(socket, &QAbstractSocket::disconnected,
                socket, &QObject::deleteLater);
        connect(socket, &QIODevice::readyRead,
                this, &LanLinkProvider::dataReceived);

    }
}

//I'm the new device and this is the answer to my UDP identity package (data received)
void LanLinkProvider::dataReceived()
{
    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());

    const QByteArray data = socket->readLine();

    //qCDebug(KDECONNECT_CORE) << "LanLinkProvider received reply:" << data;

    NetworkPackage* np = new NetworkPackage(QLatin1String(""));
    bool success = NetworkPackage::unserialize(data, np);

    if (!success) {
        delete np;
        return;
    }

    if (np->type() != PACKAGE_TYPE_IDENTITY) {
        qCWarning(KDECONNECT_CORE) << "LanLinkProvider/newConnection: Expected identity, received " << np->type();
        delete np;
        return;
    }

    // Needed in "encrypted" if ssl is used, similar to "connected"
    receivedIdentityPackages[socket].np = np;

    const QString& deviceId = np->get<QString>(QStringLiteral("deviceId"));
    //qCDebug(KDECONNECT_CORE) << "Handshaking done (i'm the new device)";

    //This socket will now be owned by the LanDeviceLink or we don't want more data to be received, forget about it
    disconnect(socket, &QIODevice::readyRead, this, &LanLinkProvider::dataReceived);

    if (np->get<int>(QStringLiteral("protocolVersion")) >= MIN_VERSION_WITH_SSL_SUPPORT) {

        bool isDeviceTrusted = KdeConnectConfig::instance()->trustedDevices().contains(deviceId);
        configureSslSocket(socket, deviceId, isDeviceTrusted);

        qCDebug(KDECONNECT_CORE) << "Starting client ssl (but I'm the server TCP socket)";

        connect(socket, &QSslSocket::encrypted, this, &LanLinkProvider::encrypted);

        if (isDeviceTrusted) {
            connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));
        }

        socket->startClientEncryption();

    } else {
        qWarning() << np->get<QString>(QStringLiteral("deviceName")) << "uses an old protocol version, this won't work";
        //addLink(deviceId, socket, np, LanDeviceLink::Locally);
        delete receivedIdentityPackages.take(socket).np;
    }
}

void LanLinkProvider::deviceLinkDestroyed(QObject* destroyedDeviceLink)
{
    const QString id = destroyedDeviceLink->property("deviceId").toString();
    //qCDebug(KDECONNECT_CORE) << "deviceLinkDestroyed" << id;
    Q_ASSERT(mLinks.key(static_cast<LanDeviceLink*>(destroyedDeviceLink)) == id);
    QMap< QString, LanDeviceLink* >::iterator linkIterator = mLinks.find(id);
    if (linkIterator != mLinks.end()) {
        Q_ASSERT(linkIterator.value() == destroyedDeviceLink);
        mLinks.erase(linkIterator);
        mPairingHandlers.take(id)->deleteLater();
    }

}

void LanLinkProvider::configureSslSocket(QSslSocket* socket, const QString& deviceId, bool isDeviceTrusted)
{
    // Setting supported ciphers manually
    // Top 3 ciphers are for new Android devices, botton two are for old Android devices
    // FIXME : These cipher suites should be checked whether they are supported or not on device
    QList<QSslCipher> socketCiphers;
    socketCiphers.append(QSslCipher(QStringLiteral("ECDHE-ECDSA-AES256-GCM-SHA384")));
    socketCiphers.append(QSslCipher(QStringLiteral("ECDHE-ECDSA-AES128-GCM-SHA256")));
    socketCiphers.append(QSslCipher(QStringLiteral("ECDHE-RSA-AES128-SHA")));
    socketCiphers.append(QSslCipher(QStringLiteral("RC4-SHA")));
    socketCiphers.append(QSslCipher(QStringLiteral("RC4-MD5")));

    // Configure for ssl
    QSslConfiguration sslConfig;
    sslConfig.setCiphers(socketCiphers);
    sslConfig.setProtocol(QSsl::TlsV1_0);

    socket->setSslConfiguration(sslConfig);
    socket->setLocalCertificate(KdeConnectConfig::instance()->certificate());
    socket->setPrivateKey(KdeConnectConfig::instance()->privateKeyPath());
    socket->setPeerVerifyName(deviceId);

    if (isDeviceTrusted) {
        QString certString = KdeConnectConfig::instance()->getDeviceProperty(deviceId, QStringLiteral("certificate"), QString());
        socket->addCaCertificate(QSslCertificate(certString.toLatin1()));
        socket->setPeerVerifyMode(QSslSocket::VerifyPeer);
    } else {
        socket->setPeerVerifyMode(QSslSocket::QueryPeer);
    }

    //Usually SSL errors are only bad for trusted devices. Uncomment this section to log errors in any case, for debugging.
    //QObject::connect(socket, static_cast<void (QSslSocket::*)(const QList<QSslError>&)>(&QSslSocket::sslErrors), [](const QList<QSslError>& errors)
    //{
    //    Q_FOREACH (const QSslError &error, errors) {
    //        qCDebug(KDECONNECT_CORE) << "SSL Error:" << error.errorString();
    //    }
    //});
}

void LanLinkProvider::configureSocket(QSslSocket* socket) {

    socket->setProxy(QNetworkProxy::NoProxy);

    socket->setSocketOption(QAbstractSocket::KeepAliveOption, QVariant(1));

    #ifdef TCP_KEEPIDLE
        // time to start sending keepalive packets (seconds)
        int maxIdle = 10;
        setsockopt(socket->socketDescriptor(), IPPROTO_TCP, TCP_KEEPIDLE, &maxIdle, sizeof(maxIdle));
    #endif

    #ifdef TCP_KEEPINTVL
        // interval between keepalive packets after the initial period (seconds)
        int interval = 5;
        setsockopt(socket->socketDescriptor(), IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    #endif

    #ifdef TCP_KEEPCNT
        // number of missed keepalive packets before disconnecting
        int count = 3;
        setsockopt(socket->socketDescriptor(), IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));
    #endif

}

void LanLinkProvider::addLink(const QString& deviceId, QSslSocket* socket, NetworkPackage* receivedPackage, LanDeviceLink::ConnectionStarted connectionOrigin)
{
    // Socket disconnection will now be handled by LanDeviceLink
    disconnect(socket, &QAbstractSocket::disconnected, socket, &QObject::deleteLater);

    LanDeviceLink* deviceLink;
    //Do we have a link for this device already?
    QMap< QString, LanDeviceLink* >::iterator linkIterator = mLinks.find(deviceId);
    if (linkIterator != mLinks.end()) {
        //qCDebug(KDECONNECT_CORE) << "Reusing link to" << deviceId;
        deviceLink = linkIterator.value();
        deviceLink->reset(socket, connectionOrigin);
    } else {
        deviceLink = new LanDeviceLink(deviceId, this, socket, connectionOrigin);
        connect(deviceLink, &QObject::destroyed, this, &LanLinkProvider::deviceLinkDestroyed);
        mLinks[deviceId] = deviceLink;
        if (mPairingHandlers.contains(deviceId)) {
            //We shouldn't have a pairinghandler if we didn't have a link.
            //Crash if debug, recover if release (by setting the new devicelink to the old pairinghandler)
            Q_ASSERT(mPairingHandlers.contains(deviceId));
            mPairingHandlers[deviceId]->setDeviceLink(deviceLink);
        }
        Q_EMIT onConnectionReceived(*receivedPackage, deviceLink);
    }

}

LanPairingHandler* LanLinkProvider::createPairingHandler(DeviceLink* link)
{
    LanPairingHandler* ph = mPairingHandlers.value(link->deviceId());
    if (!ph) {
        ph = new LanPairingHandler(link);
        qCDebug(KDECONNECT_CORE) << "creating pairing handler for" << link->deviceId();
        connect (ph, &LanPairingHandler::pairingError, link, &DeviceLink::pairingError);
        mPairingHandlers[link->deviceId()] = ph;
    }
    return ph;
}

void LanLinkProvider::userRequestsPair(const QString& deviceId)
{
    LanPairingHandler* ph = createPairingHandler(mLinks.value(deviceId));
    ph->requestPairing();
}

void LanLinkProvider::userRequestsUnpair(const QString& deviceId)
{
    LanPairingHandler* ph = createPairingHandler(mLinks.value(deviceId));
    ph->unpair();
}

void LanLinkProvider::incomingPairPackage(DeviceLink* deviceLink, const NetworkPackage& np)
{
    LanPairingHandler* ph = createPairingHandler(deviceLink);
    ph->packageReceived(np);
}

