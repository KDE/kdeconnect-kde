/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
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
#include <QMetaEnum>
#include <QNetworkProxy>
#include <QUdpSocket>
#include <QNetworkSession>
#include <QNetworkConfigurationManager>
#include <QSslCipher>
#include <QSslConfiguration>
#include <QSslKey>

#include "daemon.h"
#include "landevicelink.h"
#include "lanpairinghandler.h"
#include "kdeconnectconfig.h"
#include "qtcompat_p.h"

#define MIN_VERSION_WITH_SSL_SUPPORT 6

static const int MAX_UNPAIRED_CONNECTIONS = 42;
static const int MAX_REMEMBERED_IDENTITY_PACKETS = 42;

LanLinkProvider::LanLinkProvider(
        bool testMode,
        quint16 udpBroadcastPort,
        quint16 udpListenPort
        )
    : m_server(new Server(this))
    , m_udpSocket(this)
    , m_tcpPort(0)
    , m_udpBroadcastPort(udpBroadcastPort)
    , m_udpListenPort(udpListenPort)
    , m_testMode(testMode)
    , m_combineBroadcastsTimer(this)
{

    m_combineBroadcastsTimer.setInterval(0); // increase this if waiting a single event-loop iteration is not enough
    m_combineBroadcastsTimer.setSingleShot(true);
    connect(&m_combineBroadcastsTimer, &QTimer::timeout, this, &LanLinkProvider::broadcastToNetwork);

    connect(&m_udpSocket, &QIODevice::readyRead, this, &LanLinkProvider::udpBroadcastReceived);

    m_server->setProxy(QNetworkProxy::NoProxy);
    connect(m_server, &QTcpServer::newConnection, this, &LanLinkProvider::newConnection);

    m_udpSocket.setProxy(QNetworkProxy::NoProxy);

    //Detect when a network interface changes status, so we announce ourselves in the new network
    QNetworkConfigurationManager* networkManager = new QNetworkConfigurationManager(this);
    connect(networkManager, &QNetworkConfigurationManager::configurationChanged, this, &LanLinkProvider::onNetworkConfigurationChanged);

}

void LanLinkProvider::onNetworkConfigurationChanged(const QNetworkConfiguration& config)
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
    const QHostAddress bindAddress = m_testMode? QHostAddress::LocalHost : QHostAddress::Any;

    bool success = m_udpSocket.bind(bindAddress, m_udpListenPort, QUdpSocket::ShareAddress);
    if (!success) {
        QAbstractSocket::SocketError sockErr = m_udpSocket.error();
        // Refer to https://doc.qt.io/qt-5/qabstractsocket.html#SocketError-enum to decode socket error number
        QString errorMessage = QString::fromLatin1(QMetaEnum::fromType<QAbstractSocket::SocketError>().valueToKey(sockErr));
        qCritical(KDECONNECT_CORE)
                << QLatin1String("Failed to bind UDP socket on port")
                << m_udpListenPort
                << QLatin1String("with error")
                << errorMessage;
    }
    Q_ASSERT(success);

    m_tcpPort = MIN_TCP_PORT;
    while (!m_server->listen(bindAddress, m_tcpPort)) {
        m_tcpPort++;
        if (m_tcpPort > MAX_TCP_PORT) { //No ports available?
            qCritical(KDECONNECT_CORE) << "Error opening a port in range" << MIN_TCP_PORT << "-" << MAX_TCP_PORT;
            m_tcpPort = 0;
            return;
        }
    }

    onNetworkChange();
    qCDebug(KDECONNECT_CORE) << "LanLinkProvider started";
}

void LanLinkProvider::onStop()
{
    m_udpSocket.close();
    m_server->close();
    qCDebug(KDECONNECT_CORE) << "LanLinkProvider stopped";
}

void LanLinkProvider::onNetworkChange()
{
    if (m_combineBroadcastsTimer.isActive()) {
        qCDebug(KDECONNECT_CORE) << "Preventing duplicate broadcasts";
        return;
    }
    m_combineBroadcastsTimer.start();
}

//I'm in a new network, let's be polite and introduce myself
void LanLinkProvider::broadcastToNetwork()
{
    if (!m_server->isListening()) {
        //Not started
        return;
    }

    Q_ASSERT(m_tcpPort != 0);

    qCDebug(KDECONNECT_CORE()) << "Broadcasting identity packet";

    QList<QHostAddress> destinations = getBroadcastAddresses();

    NetworkPacket np;
    NetworkPacket::createIdentityPacket(&np);
    np.set(QStringLiteral("tcpPort"), m_tcpPort);

#ifdef Q_OS_WIN
    //On Windows we need to broadcast from every local IP address to reach all networks
    QUdpSocket sendSocket;
    sendSocket.setProxy(QNetworkProxy::NoProxy);
    for (const QNetworkInterface& iface : QNetworkInterface::allInterfaces()) {
        if ( (iface.flags() & QNetworkInterface::IsUp)
             && (iface.flags() & QNetworkInterface::IsRunning)
             && (iface.flags() & QNetworkInterface::CanBroadcast)) {
            for (const QNetworkAddressEntry& ifaceAddress : iface.addressEntries()) {
                QHostAddress sourceAddress = ifaceAddress.ip();
                if (sourceAddress.protocol() == QAbstractSocket::IPv4Protocol && sourceAddress != QHostAddress::LocalHost) {
                    qCDebug(KDECONNECT_CORE()) << "Broadcasting as" << sourceAddress;
                    sendBroadcasts(sendSocket, np, destinations);
                    sendSocket.close();
                }
            }
        }
    }
#else
    sendBroadcasts(m_udpSocket, np, destinations);
#endif
}

QList<QHostAddress> LanLinkProvider::getBroadcastAddresses()
{
    const QStringList customDevices = KdeConnectConfig::instance().customDevices();

    QList<QHostAddress> destinations;
    destinations.reserve(customDevices.length() + 1);

    // Default broadcast address
    destinations.append(m_testMode ? QHostAddress::LocalHost : QHostAddress::Broadcast);

    // Custom device addresses
    for (auto& customDevice : customDevices) {
        QHostAddress address(customDevice);
        if (address.isNull()) {
            qCWarning(KDECONNECT_CORE) << "Invalid custom device address" << customDevice;
        } else {
            destinations.append(address);
        }
    }

    return destinations;
}

void LanLinkProvider::sendBroadcasts(
        QUdpSocket& socket, const NetworkPacket& np, const QList<QHostAddress>& addresses)
{
    const QByteArray payload = np.serialize();

    for (auto& address : addresses) {
        socket.writeDatagram(payload, address, m_udpBroadcastPort);
    }
}

//I'm the existing device, a new device is kindly introducing itself.
//I will create a TcpSocket and try to connect. This can result in either tcpSocketConnected() or connectError().
void LanLinkProvider::udpBroadcastReceived()
{
    while (m_udpSocket.hasPendingDatagrams()) {

        QByteArray datagram;
        datagram.resize(m_udpSocket.pendingDatagramSize());
        QHostAddress sender;

        m_udpSocket.readDatagram(datagram.data(), datagram.size(), &sender);

        if (sender.isLoopback() && !m_testMode)
            continue;

        NetworkPacket* receivedPacket = new NetworkPacket(QLatin1String(""));
        bool success = NetworkPacket::unserialize(datagram, receivedPacket);

        //qCDebug(KDECONNECT_CORE) << "udp connection from " << receivedPacket->;

        //qCDebug(KDECONNECT_CORE) << "Datagram " << datagram.data() ;

        if (!success) {
            qCDebug(KDECONNECT_CORE) << "Could not unserialize UDP packet";
            delete receivedPacket;
            continue;
        }

        if (receivedPacket->type() != PACKET_TYPE_IDENTITY) {
            qCDebug(KDECONNECT_CORE) << "Received a UDP packet of wrong type" << receivedPacket->type();
            delete receivedPacket;
            continue;
        }

        if (receivedPacket->get<QString>(QStringLiteral("deviceId")) == KdeConnectConfig::instance().deviceId()) {
            //qCDebug(KDECONNECT_CORE) << "Ignoring my own broadcast";
            delete receivedPacket;
            continue;
        }

        int tcpPort = receivedPacket->get<int>(QStringLiteral("tcpPort"));
        if (tcpPort < MIN_TCP_PORT || tcpPort > MAX_TCP_PORT) {
            qCDebug(KDECONNECT_CORE) << "TCP port outside of kdeconnect's range";
            delete receivedPacket;
            continue;
        }

        //qCDebug(KDECONNECT_CORE) << "Received Udp identity packet from" << sender << " asking for a tcp connection on port " << tcpPort;

        if (m_receivedIdentityPackets.size() > MAX_REMEMBERED_IDENTITY_PACKETS) {
            qCWarning(KDECONNECT_CORE) << "Too many remembered identities, ignoring" << receivedPacket->get<QString>(QStringLiteral("deviceId")) << "received via UDP";
            delete receivedPacket;
            continue;
        }

        QSslSocket* socket = new QSslSocket(this);
        socket->setProxy(QNetworkProxy::NoProxy);
        m_receivedIdentityPackets[socket].np = receivedPacket;
        m_receivedIdentityPackets[socket].sender = sender;
        connect(socket, &QAbstractSocket::connected, this, &LanLinkProvider::tcpSocketConnected);
#if QT_VERSION < QT_VERSION_CHECK(5,15,0)
        connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &LanLinkProvider::connectError);
#else
        connect(socket, &QAbstractSocket::errorOccurred, this, &LanLinkProvider::connectError);
#endif
        socket->connectToHost(sender, tcpPort);
    }
}

void LanLinkProvider::connectError(QAbstractSocket::SocketError socketError)
{
    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) return;

    qCDebug(KDECONNECT_CORE) << "Socket error" << socketError;
    qCDebug(KDECONNECT_CORE) << "Fallback (1), try reverse connection (send udp packet)" << socket->errorString();
    NetworkPacket np(QLatin1String(""));
    NetworkPacket::createIdentityPacket(&np);
    np.set(QStringLiteral("tcpPort"), m_tcpPort);
    m_udpSocket.writeDatagram(np.serialize(), m_receivedIdentityPackets[socket].sender, m_udpBroadcastPort);

    //The socket we created didn't work, and we didn't manage
    //to create a LanDeviceLink from it, deleting everything.
    delete m_receivedIdentityPackets.take(socket).np;
    socket->deleteLater();
}

//We received a UDP packet and answered by connecting to them by TCP. This gets called on a successful connection.
void LanLinkProvider::tcpSocketConnected()
{
    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());

    if (!socket) return;
    // TODO Delete me?
#if QT_VERSION < QT_VERSION_CHECK(5,15,0)
    disconnect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &LanLinkProvider::connectError);
#else
    disconnect(socket, &QAbstractSocket::errorOccurred, this, &LanLinkProvider::connectError);
#endif

    configureSocket(socket);

    // If socket disconnects due to any reason after connection, link on ssl failure
    connect(socket, &QAbstractSocket::disconnected, socket, &QObject::deleteLater);

    NetworkPacket* receivedPacket = m_receivedIdentityPackets[socket].np;
    const QString& deviceId = receivedPacket->get<QString>(QStringLiteral("deviceId"));
    //qCDebug(KDECONNECT_CORE) << "tcpSocketConnected" << socket->isWritable();

    // If network is on ssl, do not believe when they are connected, believe when handshake is completed
    NetworkPacket np2(QLatin1String(""));
    NetworkPacket::createIdentityPacket(&np2);
    socket->write(np2.serialize());
    bool success = socket->waitForBytesWritten();

    if (success) {

        qCDebug(KDECONNECT_CORE) << "TCP connection done (i'm the existing device)";

        // if ssl supported
        if (receivedPacket->get<int>(QStringLiteral("protocolVersion")) >= MIN_VERSION_WITH_SSL_SUPPORT) {

            bool isDeviceTrusted = KdeConnectConfig::instance().trustedDevices().contains(deviceId);
            configureSslSocket(socket, deviceId, isDeviceTrusted);

            qCDebug(KDECONNECT_CORE) << "Starting server ssl (I'm the client TCP socket)";

            connect(socket, &QSslSocket::encrypted, this, &LanLinkProvider::encrypted);

            connect(socket, QOverload<const QList<QSslError> &>::of(&QSslSocket::sslErrors), this, &LanLinkProvider::sslErrors);

            socket->startServerEncryption();

            return; // Return statement prevents from deleting received packet, needed in slot "encrypted"
        } else {
            qWarning() << receivedPacket->get<QString>(QStringLiteral("deviceName")) << "uses an old protocol version, this won't work";
            //addLink(deviceId, socket, receivedPacket, LanDeviceLink::Remotely);
        }

    } else {
        //I think this will never happen, but if it happens the deviceLink
        //(or the socket that is now inside it) might not be valid. Delete them.
        qCDebug(KDECONNECT_CORE) << "Fallback (2), try reverse connection (send udp packet)";
        m_udpSocket.writeDatagram(np2.serialize(), m_receivedIdentityPackets[socket].sender, m_udpBroadcastPort);
    }

    delete m_receivedIdentityPackets.take(socket).np;
    //We don't delete the socket because now it's owned by the LanDeviceLink
}

void LanLinkProvider::encrypted()
{
    qCDebug(KDECONNECT_CORE) << "Socket successfully established an SSL connection";

    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) return;

    Q_ASSERT(socket->mode() != QSslSocket::UnencryptedMode);
    LanDeviceLink::ConnectionStarted connectionOrigin = (socket->mode() == QSslSocket::SslClientMode)? LanDeviceLink::Locally : LanDeviceLink::Remotely;

    NetworkPacket* receivedPacket = m_receivedIdentityPackets[socket].np;
    const QString& deviceId = receivedPacket->get<QString>(QStringLiteral("deviceId"));

    addLink(deviceId, socket, receivedPacket, connectionOrigin);

    // Copied from tcpSocketConnected slot, now delete received packet
    delete m_receivedIdentityPackets.take(socket).np;
}

void LanLinkProvider::sslErrors(const QList<QSslError>& errors)
{
    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) return;

    bool fatal = false;
    for (const QSslError& error : errors) {
        if (error.error() != QSslError::SelfSignedCertificate) {
            qCCritical(KDECONNECT_CORE) << "Disconnecting due to fatal SSL Error: " << error;
            fatal = true;
        } else {
            qCDebug(KDECONNECT_CORE) << "Ignoring self-signed cert error";
        }
    }

    if (fatal) {
        socket->disconnectFromHost();
        delete m_receivedIdentityPackets.take(socket).np;
    }
}

//I'm the new device and this is the answer to my UDP identity packet (no data received yet). They are connecting to us through TCP, and they should send an identity.
void LanLinkProvider::newConnection()
{
    qCDebug(KDECONNECT_CORE) << "LanLinkProvider newConnection";

    while (m_server->hasPendingConnections()) {
        QSslSocket* socket = m_server->nextPendingConnection();
        configureSocket(socket);
        //This socket is still managed by us (and child of the QTcpServer), if
        //it disconnects before we manage to pass it to a LanDeviceLink, it's
        //our responsibility to delete it. We do so with this connection.
        connect(socket, &QAbstractSocket::disconnected,
                socket, &QObject::deleteLater);
        connect(socket, &QIODevice::readyRead,
                this, &LanLinkProvider::dataReceived);

        QTimer* timer = new QTimer(socket);
        timer->setSingleShot(true);
        timer->setInterval(1000);
        connect(socket, &QSslSocket::encrypted,
                timer, &QObject::deleteLater);
        connect(timer, &QTimer::timeout, socket, [socket] {
            qCWarning(KDECONNECT_CORE) << "LanLinkProvider/newConnection: Host timed out without sending any identity." << socket->peerAddress();
            socket->disconnectFromHost();
        });
        timer->start();
    }
}

//I'm the new device and this is the answer to my UDP identity packet (data received)
void LanLinkProvider::dataReceived()
{
    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());
    //the size here is arbitrary and is now at 8192 bytes. It needs to be considerably long as it includes the capabilities but there needs to be a limit
    //Tested between my systems and I get around 2000 per identity package.
    if (socket->bytesAvailable() > 8192) {
        qCWarning(KDECONNECT_CORE) << "LanLinkProvider/newConnection: Suspiciously long identity package received. Closing connection." << socket->peerAddress() << socket->bytesAvailable();
        socket->disconnectFromHost();
        return;
    }

#if QT_VERSION < QT_VERSION_CHECK(5,7,0)
    if (!socket->canReadLine())
        return;
#else
    socket->startTransaction();
#endif

    const QByteArray data = socket->readLine();

    qCDebug(KDECONNECT_CORE) << "LanLinkProvider received reply:" << data;

    NetworkPacket* np = new NetworkPacket(QLatin1String(""));
    bool success = NetworkPacket::unserialize(data, np);

#if QT_VERSION < QT_VERSION_CHECK(5,7,0)
    if (!success) {
        delete np;
        return;
    }
#else
    if (!success) {
        delete np;
        socket->rollbackTransaction();
        return;
    }
    socket->commitTransaction();
#endif

    if (np->type() != PACKET_TYPE_IDENTITY) {
        qCWarning(KDECONNECT_CORE) << "LanLinkProvider/newConnection: Expected identity, received " << np->type();
        delete np;
        return;
    }

    if (m_receivedIdentityPackets.size() > MAX_REMEMBERED_IDENTITY_PACKETS) {
        qCWarning(KDECONNECT_CORE) << "Too many remembered identities, ignoring" << np->get<QString>(QStringLiteral("deviceId")) << "received via TCP";
        delete np;
        return;
    }

    // Needed in "encrypted" if ssl is used, similar to "tcpSocketConnected"
    m_receivedIdentityPackets[socket].np = np;

    const QString& deviceId = np->get<QString>(QStringLiteral("deviceId"));
    //qCDebug(KDECONNECT_CORE) << "Handshaking done (i'm the new device)";

    //This socket will now be owned by the LanDeviceLink or we don't want more data to be received, forget about it
    disconnect(socket, &QIODevice::readyRead, this, &LanLinkProvider::dataReceived);

    if (np->get<int>(QStringLiteral("protocolVersion")) >= MIN_VERSION_WITH_SSL_SUPPORT) {

        bool isDeviceTrusted = KdeConnectConfig::instance().trustedDevices().contains(deviceId);
        configureSslSocket(socket, deviceId, isDeviceTrusted);

        qCDebug(KDECONNECT_CORE) << "Starting client ssl (but I'm the server TCP socket)";

        connect(socket, &QSslSocket::encrypted, this, &LanLinkProvider::encrypted);

        if (isDeviceTrusted) {
            connect(socket, QOverload<const QList<QSslError> &>::of(&QSslSocket::sslErrors), this, &LanLinkProvider::sslErrors);
        }

        socket->startClientEncryption();

    } else {
        qWarning() << np->get<QString>(QStringLiteral("deviceName")) << "uses an old protocol version, this won't work";
        //addLink(deviceId, socket, np, LanDeviceLink::Locally);
        delete m_receivedIdentityPackets.take(socket).np;
    }
}

void LanLinkProvider::deviceLinkDestroyed(QObject* destroyedDeviceLink)
{
    const QString id = destroyedDeviceLink->property("deviceId").toString();
    //qCDebug(KDECONNECT_CORE) << "deviceLinkDestroyed" << id;
    QMap< QString, LanDeviceLink* >::iterator linkIterator = m_links.find(id);
    Q_ASSERT(linkIterator != m_links.end());
    if (linkIterator != m_links.end()) {
        Q_ASSERT(linkIterator.value() == destroyedDeviceLink);
        m_links.erase(linkIterator);
        auto pairingHandler = m_pairingHandlers.take(id);
        if (pairingHandler) {
            pairingHandler->deleteLater();
        }
    }

}

void LanLinkProvider::configureSslSocket(QSslSocket* socket, const QString& deviceId, bool isDeviceTrusted)
{
    // Setting supported ciphers manually, to match those on Android (FIXME: Test if this can be left unconfigured and still works for Android 4)
    QList<QSslCipher> socketCiphers;
    socketCiphers.append(QSslCipher(QStringLiteral("ECDHE-ECDSA-AES256-GCM-SHA384")));
    socketCiphers.append(QSslCipher(QStringLiteral("ECDHE-ECDSA-AES128-GCM-SHA256")));
    socketCiphers.append(QSslCipher(QStringLiteral("ECDHE-RSA-AES128-SHA")));

    // Configure for ssl
    QSslConfiguration sslConfig;
    sslConfig.setCiphers(socketCiphers);
    sslConfig.setLocalCertificate(KdeConnectConfig::instance().certificate());

    QFile privateKeyFile(KdeConnectConfig::instance().privateKeyPath());
    QSslKey privateKey;
    if (privateKeyFile.open(QIODevice::ReadOnly)) {
        privateKey = QSslKey(privateKeyFile.readAll(), QSsl::Rsa);
    }
    privateKeyFile.close();
    sslConfig.setPrivateKey(privateKey);

    if (isDeviceTrusted) {
        QString certString = KdeConnectConfig::instance().getDeviceProperty(deviceId, QStringLiteral("certificate"), QString());
        sslConfig.setCaCertificates({QSslCertificate(certString.toLatin1())});
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyPeer);
    } else {
        sslConfig.setPeerVerifyMode(QSslSocket::QueryPeer);
    }
    socket->setSslConfiguration(sslConfig);
    socket->setPeerVerifyName(deviceId);


    //Usually SSL errors are only bad for trusted devices. Uncomment this section to log errors in any case, for debugging.
    //QObject::connect(socket, static_cast<void (QSslSocket::*)(const QList<QSslError>&)>(&QSslSocket::sslErrors), [](const QList<QSslError>& errors)
    //{
    //    Q_FOREACH (const QSslError& error, errors) {
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

void LanLinkProvider::addLink(const QString& deviceId, QSslSocket* socket, NetworkPacket* receivedPacket, LanDeviceLink::ConnectionStarted connectionOrigin)
{
    // Socket disconnection will now be handled by LanDeviceLink
    disconnect(socket, &QAbstractSocket::disconnected, socket, &QObject::deleteLater);

    LanDeviceLink* deviceLink;
    //Do we have a link for this device already?
    QMap< QString, LanDeviceLink* >::iterator linkIterator = m_links.find(deviceId);
    if (linkIterator != m_links.end()) {
        //qCDebug(KDECONNECT_CORE) << "Reusing link to" << deviceId;
        deviceLink = linkIterator.value();
        deviceLink->reset(socket, connectionOrigin);
    } else {
        deviceLink = new LanDeviceLink(deviceId, this, socket, connectionOrigin);
        // Socket disconnection will now be handled by LanDeviceLink
        disconnect(socket, &QAbstractSocket::disconnected, socket, &QObject::deleteLater);
        bool isDeviceTrusted = KdeConnectConfig::instance().trustedDevices().contains(deviceId);
        if (!isDeviceTrusted && m_links.size() > MAX_UNPAIRED_CONNECTIONS) {
            qCWarning(KDECONNECT_CORE) << "Too many unpaired devices to remember them all. Ignoring " << deviceId;
            socket->disconnectFromHost();
            socket->deleteLater();
            return;
        }
        connect(deviceLink, &QObject::destroyed, this, &LanLinkProvider::deviceLinkDestroyed);
        m_links[deviceId] = deviceLink;
        if (m_pairingHandlers.contains(deviceId)) {
            //We shouldn't have a pairinghandler if we didn't have a link.
            //Crash if debug, recover if release (by setting the new devicelink to the old pairinghandler)
            Q_ASSERT(m_pairingHandlers.contains(deviceId));
            m_pairingHandlers[deviceId]->setDeviceLink(deviceLink);
        }
    }
    Q_EMIT onConnectionReceived(*receivedPacket, deviceLink);
}

LanPairingHandler* LanLinkProvider::createPairingHandler(DeviceLink* link)
{
    LanPairingHandler* ph = m_pairingHandlers.value(link->deviceId());
    if (!ph) {
        ph = new LanPairingHandler(link);
        qCDebug(KDECONNECT_CORE) << "creating pairing handler for" << link->deviceId();
        connect (ph, &LanPairingHandler::pairingError, link, &DeviceLink::pairingError);
        m_pairingHandlers[link->deviceId()] = ph;
    }
    return ph;
}

void LanLinkProvider::userRequestsPair(const QString& deviceId)
{
    LanPairingHandler* ph = createPairingHandler(m_links.value(deviceId));
    ph->requestPairing();
}

void LanLinkProvider::userRequestsUnpair(const QString& deviceId)
{
    LanPairingHandler* ph = createPairingHandler(m_links.value(deviceId));
    ph->unpair();
}

void LanLinkProvider::incomingPairPacket(DeviceLink* deviceLink, const NetworkPacket& np)
{
    LanPairingHandler* ph = createPairingHandler(deviceLink);
    ph->packetReceived(np);
}

