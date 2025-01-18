/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "lanlinkprovider.h"
#include "core_debug.h"

#ifndef Q_OS_WIN
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
// Winsock2 needs to be included before any other header
#include <mstcpip.h>
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_FREEBSD)
#include <QNetworkInterface>
#endif

#include <QHostInfo>
#include <QMetaEnum>
#include <QNetworkProxy>
#include <QSslCipher>
#include <QSslConfiguration>
#include <QSslKey>
#include <QStringList>
#include <QTcpServer>
#include <QUdpSocket>

#include "daemon.h"
#include "dbushelper.h"
#include "kdeconnectconfig.h"
#include "landevicelink.h"

static const int MAX_UNPAIRED_CONNECTIONS = 42;
static const int MAX_REMEMBERED_IDENTITY_PACKETS = 42;

static const long MILLIS_DELAY_BETWEEN_CONNECTIONS_TO_SAME_DEVICE = 500;

LanLinkProvider::LanLinkProvider(bool testMode, bool isDisabled)
    : m_server(new Server(this))
    , m_udpSocket(this)
    , m_tcpPort(0)
    , m_testMode(testMode)
    , m_combineNetworkChangeTimer(this)
    , m_disabled(isDisabled)
#ifdef KDECONNECT_MDNS
    , m_mdnsDiscovery(this)
#endif
{
    m_combineNetworkChangeTimer.setInterval(0); // increase this if waiting a single event-loop iteration is not enough
    m_combineNetworkChangeTimer.setSingleShot(true);
    connect(&m_combineNetworkChangeTimer, &QTimer::timeout, this, &LanLinkProvider::combinedOnNetworkChange);

    connect(&m_udpSocket, &QIODevice::readyRead, this, &LanLinkProvider::udpBroadcastReceived);

    m_server->setProxy(QNetworkProxy::NoProxy);
    connect(m_server, &QTcpServer::newConnection, this, &LanLinkProvider::newConnection);

    m_udpSocket.setProxy(QNetworkProxy::NoProxy);

    connect(&m_udpSocket, &QAbstractSocket::errorOccurred, [](QAbstractSocket::SocketError socketError) {
        qWarning() << "Error sending UDP packet:" << socketError;
    });

    const auto checkNetworkChange = [this]() {
        if (QNetworkInformation::instance()->reachability() == QNetworkInformation::Reachability::Online) {
            onNetworkChange();
        }
    };
    // Detect when a network interface changes status, so we announce ourselves in the new network
    QNetworkInformation::instance()->loadBackendByFeatures(QNetworkInformation::Feature::Reachability);

    // We want to know if our current network reachability has changed, or if we change from one network to another
    connect(QNetworkInformation::instance(), &QNetworkInformation::reachabilityChanged, this, checkNetworkChange);
    connect(QNetworkInformation::instance(), &QNetworkInformation::transportMediumChanged, this, checkNetworkChange);
}

LanLinkProvider::~LanLinkProvider()
{
}

void LanLinkProvider::enable()
{
    if (m_disabled == true) {
        m_disabled = false;
        onStart();
    }
}

void LanLinkProvider::disable()
{
    if (m_disabled == false) {
        onStop();
        m_disabled = true;
    }
}

void LanLinkProvider::onStart()
{
    if (m_disabled) {
        return;
    }

    const QHostAddress bindAddress = m_testMode ? QHostAddress::LocalHost : QHostAddress::Any;

    bool success = m_udpSocket.bind(bindAddress, UDP_PORT, QUdpSocket::ShareAddress);
    if (!success) {
        QAbstractSocket::SocketError sockErr = m_udpSocket.error();
        // Refer to https://doc.qt.io/qt-5/qabstractsocket.html#SocketError-enum to decode socket error number
        QString errorMessage = QString::fromLatin1(QMetaEnum::fromType<QAbstractSocket::SocketError>().valueToKey(sockErr));
        qCritical(KDECONNECT_CORE) << "Failed to bind UDP socket on port" << UDP_PORT << "with error" << errorMessage;
    }
    Q_ASSERT(success);

    m_tcpPort = MIN_TCP_PORT;
    while (!m_server->listen(bindAddress, m_tcpPort)) {
        m_tcpPort++;
        if (m_tcpPort > MAX_TCP_PORT) { // No ports available?
            qCritical(KDECONNECT_CORE) << "Error opening a port in range" << MIN_TCP_PORT << "-" << MAX_TCP_PORT;
            m_tcpPort = 0;
            return;
        }
    }

    broadcastUdpIdentityPacket();

#ifdef KDECONNECT_MDNS
    m_mdnsDiscovery.onStart();
#endif

    qCDebug(KDECONNECT_CORE) << "LanLinkProvider started";
}

void LanLinkProvider::onStop()
{
    if (m_disabled) {
        return;
    }
#ifdef KDECONNECT_MDNS
    m_mdnsDiscovery.onStop();
#endif
    m_udpSocket.close();
    m_server->close();
    qCDebug(KDECONNECT_CORE) << "LanLinkProvider stopped";
}

void LanLinkProvider::onNetworkChange()
{
    if (m_disabled) {
        return;
    }
    if (m_combineNetworkChangeTimer.isActive()) {
        qCDebug(KDECONNECT_CORE) << "Device discovery triggered too fast, ignoring";
        return;
    }
    m_combineNetworkChangeTimer.start();
}

// I'm in a new network, let's be polite and introduce myself
void LanLinkProvider::combinedOnNetworkChange()
{
    if (m_disabled) {
        return;
    }
    if (!m_server->isListening()) {
        qWarning() << "TCP server not listening, not broadcasting";
        return;
    }

    Q_ASSERT(m_tcpPort != 0);

    broadcastUdpIdentityPacket();
#ifdef KDECONNECT_MDNS
    m_mdnsDiscovery.onNetworkChange();
#endif
}

void LanLinkProvider::broadcastUdpIdentityPacket()
{
    if (qEnvironmentVariableIsSet("KDECONNECT_DISABLE_UDP_BROADCAST")) {
        qWarning() << "Not broadcasting UDP because KDECONNECT_DISABLE_UDP_BROADCAST is set";
        return;
    }
    qCDebug(KDECONNECT_CORE) << "Broadcasting identity packet";

    QList<QHostAddress> addresses = getBroadcastAddresses();

#if defined(Q_OS_WIN) || defined(Q_OS_FREEBSD)
    // On Windows and FreeBSD we need to broadcast from every local IP address to reach all networks
    QUdpSocket sendSocket;
    sendSocket.setProxy(QNetworkProxy::NoProxy);
    for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
        if ((iface.flags() & QNetworkInterface::IsUp) && (iface.flags() & QNetworkInterface::IsRunning) && (iface.flags() & QNetworkInterface::CanBroadcast)) {
            for (const QNetworkAddressEntry &ifaceAddress : iface.addressEntries()) {
                QHostAddress sourceAddress = ifaceAddress.ip();
                if (sourceAddress.protocol() == QAbstractSocket::IPv4Protocol && sourceAddress != QHostAddress::LocalHost) {
                    qCDebug(KDECONNECT_CORE) << "Broadcasting as" << sourceAddress;
                    sendSocket.bind(sourceAddress);
                    sendUdpIdentityPacket(sendSocket, addresses);
                    sendSocket.close();
                }
            }
        }
    }
#else
    sendUdpIdentityPacket(addresses);
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
    for (auto &customDevice : customDevices) {
        QHostAddress address(customDevice);
        if (address.isNull()) {
            qCWarning(KDECONNECT_CORE) << "Invalid custom device address" << customDevice;
        } else {
            destinations.append(address);
        }
    }

    return destinations;
}

void LanLinkProvider::sendUdpIdentityPacket(const QList<QHostAddress> &addresses)
{
    sendUdpIdentityPacket(m_udpSocket, addresses);
}

void LanLinkProvider::sendUdpIdentityPacket(QUdpSocket &socket, const QList<QHostAddress> &addresses)
{
    DeviceInfo myDeviceInfo = KdeConnectConfig::instance().deviceInfo();
    NetworkPacket identityPacket = myDeviceInfo.toIdentityPacket();
    identityPacket.set(QStringLiteral("tcpPort"), m_tcpPort);
    const QByteArray payload = identityPacket.serialize();

    for (auto &address : addresses) {
        qint64 bytes = socket.writeDatagram(payload, address, UDP_PORT);
        if (bytes == -1 && socket.error() == QAbstractSocket::DatagramTooLargeError) {
            // On macOS and FreeBSD, UDP broadcasts larger than MTU get dropped. See:
            // https://opensource.apple.com/source/xnu/xnu-3789.1.32/bsd/netinet/ip_output.c.auto.html#:~:text=/*%20don%27t%20allow%20broadcast%20messages%20to%20be%20fragmented%20*/
            // We remove the capabilities to reduce the size of the packet.
            // This should only happen for broadcasts, so UDP packets sent from MDNS discoveries should still work.
            qWarning() << "Identity packet to" << address << "got rejected because it was too large. Retrying without including the capabilities";
            identityPacket.set(QStringLiteral("outgoingCapabilities"), QStringList());
            identityPacket.set(QStringLiteral("incomingCapabilities"), QStringList());
            const QByteArray smallPayload = identityPacket.serialize();
            socket.writeDatagram(smallPayload, address, UDP_PORT);
        }
    }
}

// I'm the existing device, a new device is kindly introducing itself.
// I will create a TcpSocket and try to connect. This can result in either tcpSocketConnected() or connectError().
void LanLinkProvider::udpBroadcastReceived()
{
    while (m_udpSocket.hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_udpSocket.pendingDatagramSize());
        QHostAddress sender;

        m_udpSocket.readDatagram(datagram.data(), datagram.size(), &sender);

        if (sender.isLoopback() && !m_testMode)
            continue;

        NetworkPacket *receivedPacket = new NetworkPacket();
        bool success = NetworkPacket::unserialize(datagram, receivedPacket);

        // qCDebug(KDECONNECT_CORE) << "Datagram " << datagram.data() ;

        if (!success) {
            qCDebug(KDECONNECT_CORE) << "Could not unserialize UDP packet";
            delete receivedPacket;
            continue;
        }

        if (!DeviceInfo::isValidIdentityPacket(receivedPacket)) {
            qCWarning(KDECONNECT_CORE) << "Invalid identity packet received";
            delete receivedPacket;
            continue;
        }

        QString deviceId = receivedPacket->get<QString>(QStringLiteral("deviceId"));

        if (deviceId == KdeConnectConfig::instance().deviceId()) {
            // qCDebug(KDECONNECT_CORE) << "Ignoring my own broadcast";
            delete receivedPacket;
            continue;
        }

        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (m_lastConnectionTime[deviceId] + MILLIS_DELAY_BETWEEN_CONNECTIONS_TO_SAME_DEVICE > now) {
            qCDebug(KDECONNECT_CORE) << "Discarding second UPD packet from the same device" << deviceId << "received too quickly";
            delete receivedPacket;
            return;
        }
        m_lastConnectionTime[deviceId] = now;

        int tcpPort = receivedPacket->get<int>(QStringLiteral("tcpPort"));
        if (tcpPort < MIN_TCP_PORT || tcpPort > MAX_TCP_PORT) {
            qCDebug(KDECONNECT_CORE) << "TCP port outside of kdeconnect's range";
            delete receivedPacket;
            continue;
        }

        bool isDeviceTrusted = KdeConnectConfig::instance().trustedDevices().contains(deviceId);
        int protocolVersion = receivedPacket->get<int>(QStringLiteral("protocolVersion"), 0);
        if (isDeviceTrusted && isProtocolDowngrade(deviceId, protocolVersion)) {
            qCWarning(KDECONNECT_CORE) << "Refusing to connect to a device using an older protocol version. Ignoring " << deviceId;
            delete receivedPacket;
            return;
        }

        // qCDebug(KDECONNECT_CORE) << "Received Udp identity packet from" << sender << " asking for a tcp connection on port " << tcpPort;

        if (m_receivedIdentityPackets.size() > MAX_REMEMBERED_IDENTITY_PACKETS) {
            qCWarning(KDECONNECT_CORE) << "Too many remembered identities, ignoring" << receivedPacket->get<QString>(QStringLiteral("deviceId"))
                                       << "received via UDP";
            delete receivedPacket;
            continue;
        }

        QSslSocket *socket = new QSslSocket(this);
        socket->setProxy(QNetworkProxy::NoProxy);
        m_receivedIdentityPackets[socket].np = receivedPacket;
        m_receivedIdentityPackets[socket].sender = sender;
        connect(socket, &QAbstractSocket::connected, this, &LanLinkProvider::tcpSocketConnected);
        connect(socket, &QAbstractSocket::errorOccurred, this, &LanLinkProvider::connectError);
        connect(socket, &QObject::destroyed, this, [this, socket]() {
            delete m_receivedIdentityPackets.take(socket).np;
        });
        socket->connectToHost(sender, tcpPort);
    }
}

void LanLinkProvider::connectError(QAbstractSocket::SocketError socketError)
{
    QSslSocket *socket = qobject_cast<QSslSocket *>(sender());
    if (!socket)
        return;

    qCDebug(KDECONNECT_CORE) << "Socket error" << socketError;
    qCDebug(KDECONNECT_CORE) << "Fallback (1), try reverse connection (send udp packet)" << socket->errorString();
    NetworkPacket np = KdeConnectConfig::instance().deviceInfo().toIdentityPacket();
    np.set(QStringLiteral("tcpPort"), m_tcpPort);
    m_udpSocket.writeDatagram(np.serialize(), m_receivedIdentityPackets[socket].sender, UDP_PORT);

    // The socket we created didn't work, and we didn't manage
    // to create a LanDeviceLink from it, deleting everything.
    socket->deleteLater();
}

// We received a UDP packet and answered by connecting to them by TCP. This gets called on a successful connection.
void LanLinkProvider::tcpSocketConnected()
{
    QSslSocket *socket = qobject_cast<QSslSocket *>(sender());

    if (!socket) {
        return;
    }

    disconnect(socket, &QAbstractSocket::errorOccurred, this, &LanLinkProvider::connectError);

    configureSocket(socket);

    // If socket disconnects due to any reason after connection, link on ssl failure
    connect(socket, &QAbstractSocket::disconnected, socket, &QObject::deleteLater);

    NetworkPacket *receivedPacket = m_receivedIdentityPackets[socket].np;
    const QString &deviceId = receivedPacket->get<QString>(QStringLiteral("deviceId"));
    // qCDebug(KDECONNECT_CORE) << "tcpSocketConnected" << socket->isWritable();

    // If network is on ssl, do not believe when they are connected, believe when handshake is completed
    NetworkPacket np2 = KdeConnectConfig::instance().deviceInfo().toIdentityPacket();
    socket->write(np2.serialize());
    bool success = socket->waitForBytesWritten();

    if (success) {
        qCDebug(KDECONNECT_CORE) << "TCP connection done (i'm the existing device)";

        // if ssl supported
        bool isDeviceTrusted = KdeConnectConfig::instance().trustedDevices().contains(deviceId);
        configureSslSocket(socket, deviceId, isDeviceTrusted);

        qCDebug(KDECONNECT_CORE) << "Starting server ssl (I'm the client TCP socket)";

        connect(socket, &QSslSocket::encrypted, this, &LanLinkProvider::encrypted);

        connect(socket, &QSslSocket::sslErrors, this, &LanLinkProvider::sslErrors);

        socket->startServerEncryption();
    } else {
        // The socket doesn't seem to work, so we can't create the connection.

        qCDebug(KDECONNECT_CORE) << "Fallback (2), try reverse connection (send udp packet)";
        m_udpSocket.writeDatagram(np2.serialize(), m_receivedIdentityPackets[socket].sender, UDP_PORT);

        // Disconnect should trigger deleteLater, which should remove the socket from m_receivedIdentityPackets
        socket->disconnectFromHost();
    }
}

void LanLinkProvider::encrypted()
{
    qCDebug(KDECONNECT_CORE) << "Socket successfully established an SSL connection";

    QSslSocket *socket = qobject_cast<QSslSocket *>(sender());
    if (!socket)
        return;

    Q_ASSERT(socket->mode() != QSslSocket::UnencryptedMode);

    NetworkPacket *identityPacket = m_receivedIdentityPackets[socket].np;

    int protocolVersion = identityPacket->get<int>(QStringLiteral("protocolVersion"), -1);
    if (protocolVersion == 8) {
        disconnect(socket, &QObject::destroyed, nullptr, nullptr);
        delete m_receivedIdentityPackets.take(socket).np;

        NetworkPacket myIdentity = KdeConnectConfig::instance().deviceInfo().toIdentityPacket();
        socket->write(myIdentity.serialize());
        socket->flush();
        connect(socket, &QIODevice::readyRead, this, [this, socket]() {
            if (!socket->canReadLine()) {
                // This can happen if the packet is large enough to be split in two chunks
                return;
            }
            disconnect(socket, &QIODevice::readyRead, nullptr, nullptr);
            QByteArray identityString = socket->readLine();
            NetworkPacket *identityPacket = new NetworkPacket();
            bool success = NetworkPacket::unserialize(identityString, identityPacket);
            if (!success || !DeviceInfo::isValidIdentityPacket(identityPacket)) {
                qCWarning(KDECONNECT_CORE, "Remote device doesn't correctly implement protocol version 8");
                disconnect(socket, &QObject::destroyed, nullptr, nullptr);
                return;
            }
            DeviceInfo deviceInfo = DeviceInfo::FromIdentityPacketAndCert(*identityPacket, socket->peerCertificate());

            // We don't delete the socket because now it's owned by the LanDeviceLink
            disconnect(socket, &QObject::destroyed, nullptr, nullptr);
            addLink(socket, deviceInfo);
        });
    } else {
        DeviceInfo deviceInfo = DeviceInfo::FromIdentityPacketAndCert(*identityPacket, socket->peerCertificate());

        disconnect(socket, &QObject::destroyed, nullptr, nullptr);
        delete m_receivedIdentityPackets.take(socket).np;

        addLink(socket, deviceInfo);
    }
}

void LanLinkProvider::sslErrors(const QList<QSslError> &errors)
{
    QSslSocket *socket = qobject_cast<QSslSocket *>(sender());
    if (!socket)
        return;

    bool fatal = false;
    for (const QSslError &error : errors) {
        if (error.error() != QSslError::SelfSignedCertificate) {
            qCCritical(KDECONNECT_CORE) << "Disconnecting due to fatal SSL Error: " << error;
            fatal = true;
        } else {
            qCDebug(KDECONNECT_CORE) << "Ignoring self-signed cert error";
        }
    }

    if (fatal) {
        // Disconnect should trigger deleteLater, which should remove the socket from m_receivedIdentityPackets
        socket->disconnectFromHost();
    }
}

// I'm the new device and this is the answer to my UDP identity packet (no data received yet). They are connecting to us through TCP, and they should send an
// identity.
void LanLinkProvider::newConnection()
{
    qCDebug(KDECONNECT_CORE) << "LanLinkProvider newConnection";

    while (m_server->hasPendingConnections()) {
        QSslSocket *socket = m_server->nextPendingConnection();
        configureSocket(socket);
        // This socket is still managed by us (and child of the QTcpServer), if
        // it disconnects before we manage to pass it to a LanDeviceLink, it's
        // our responsibility to delete it. We do so with this connection.
        connect(socket, &QAbstractSocket::disconnected, socket, &QObject::deleteLater);
        connect(socket, &QIODevice::readyRead, this, &LanLinkProvider::dataReceived);

        QTimer *timer = new QTimer(socket);
        timer->setSingleShot(true);
        timer->setInterval(1000);
        connect(socket, &QSslSocket::encrypted, timer, &QObject::deleteLater);
        connect(timer, &QTimer::timeout, socket, [socket] {
            qCWarning(KDECONNECT_CORE) << "LanLinkProvider/newConnection: Host timed out without sending any identity." << socket->peerAddress();
            socket->disconnectFromHost();
        });
        timer->start();
    }
}

// I'm the new device and this is the TCP response to my UDP identity packet
void LanLinkProvider::dataReceived()
{
    QSslSocket *socket = qobject_cast<QSslSocket *>(sender());
    // the size here is arbitrary and is now at 8192 bytes. It needs to be considerably long as it includes the capabilities but there needs to be a limit
    // Tested between my systems and I get around 2000 per identity package.
    if (socket->bytesAvailable() > 8192) {
        qCWarning(KDECONNECT_CORE) << "LanLinkProvider/newConnection: Suspiciously long identity package received. Closing connection." << socket->peerAddress()
                                   << socket->bytesAvailable();
        socket->disconnectFromHost();
        return;
    }

    if (!socket->canReadLine()) {
        // This can happen if the packet is large enough to be split in two chunks
        return;
    }

    const QByteArray data = socket->readLine();

    qCDebug(KDECONNECT_CORE) << "LanLinkProvider received reply:" << data;

    NetworkPacket *np = new NetworkPacket();
    bool success = NetworkPacket::unserialize(data, np);

    if (!success) {
        delete np;
        return;
    }

    if (!DeviceInfo::isValidIdentityPacket(np)) {
        qCWarning(KDECONNECT_CORE) << "Invalid identity packet received";
        delete np;
        return;
    }

    const QString &deviceId = np->get<QString>(QStringLiteral("deviceId"));

    if (m_receivedIdentityPackets.size() > MAX_REMEMBERED_IDENTITY_PACKETS) {
        qCWarning(KDECONNECT_CORE) << "Too many remembered identities, ignoring" << deviceId << "received via TCP";
        delete np;
        return;
    }

    bool isDeviceTrusted = KdeConnectConfig::instance().trustedDevices().contains(deviceId);
    int protocolVersion = np->get<int>(QStringLiteral("protocolVersion"), 0);
    if (isDeviceTrusted && isProtocolDowngrade(deviceId, protocolVersion)) {
        qCWarning(KDECONNECT_CORE) << "Refusing to connect to a device using an older protocol version. Ignoring " << deviceId;
        delete np;
        return;
    }

    // Needed in "encrypted" if ssl is used, similar to "tcpSocketConnected"
    m_receivedIdentityPackets[socket].np = np;
    connect(socket, &QObject::destroyed, this, [this, socket]() {
        delete m_receivedIdentityPackets.take(socket).np;
    });

    // qCDebug(KDECONNECT_CORE) << "Handshaking done (i'm the new device)";

    // This socket will now be owned by the LanDeviceLink or we don't want more data to be received, forget about it
    disconnect(socket, &QIODevice::readyRead, this, &LanLinkProvider::dataReceived);

    configureSslSocket(socket, deviceId, isDeviceTrusted);

    qCDebug(KDECONNECT_CORE) << "Starting client ssl (but I'm the server TCP socket)";

    connect(socket, &QSslSocket::encrypted, this, &LanLinkProvider::encrypted);

    if (isDeviceTrusted) {
        connect(socket, &QSslSocket::sslErrors, this, &LanLinkProvider::sslErrors);
    }

    socket->startClientEncryption(); // Will call encrypted()
}

bool LanLinkProvider::isProtocolDowngrade(const QString &deviceId, int protocolVersion) const
{
    int lastKnownProtocolVersion = KdeConnectConfig::instance().getTrustedDeviceProtocolVersion(deviceId);
    return lastKnownProtocolVersion > protocolVersion;
}

void LanLinkProvider::onLinkDestroyed(const QString &deviceId, DeviceLink *oldPtr)
{
    qCDebug(KDECONNECT_CORE) << "LanLinkProvider deviceLinkDestroyed" << deviceId;
    DeviceLink *link = m_links.take(deviceId);
    Q_ASSERT(link == oldPtr);
}

void LanLinkProvider::configureSslSocket(QSslSocket *socket, const QString &deviceId, bool isDeviceTrusted)
{
    // Configure for ssl
    QSslConfiguration sslConfig;
    sslConfig.setLocalCertificate(KdeConnectConfig::instance().certificate());
    sslConfig.setPrivateKey(KdeConnectConfig::instance().privateKey());

    if (isDeviceTrusted) {
        QSslCertificate certificate = KdeConnectConfig::instance().getTrustedDeviceCertificate(deviceId);
        sslConfig.setCaCertificates({certificate});
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyPeer);
    } else {
        sslConfig.setPeerVerifyMode(QSslSocket::QueryPeer);
    }
    socket->setSslConfiguration(sslConfig);
    socket->setPeerVerifyName(deviceId);

    // Usually SSL errors are only bad for trusted devices. Uncomment this section to log errors in any case, for debugging.
    // connect(socket, &QSslSocket::sslErrors, [](const QList<QSslError>& errors)
    // {
    //      for (const QSslError& error : errors) {
    //          qCDebug(KDECONNECT_CORE) << "SSL Error:" << error.errorString();
    //      }
    // });
}

void LanLinkProvider::configureSocket(QSslSocket *socket)
{
    socket->setProxy(QNetworkProxy::NoProxy);
    socket->setSocketOption(QAbstractSocket::KeepAliveOption, QVariant(1));
}

void LanLinkProvider::addLink(QSslSocket *socket, const DeviceInfo &deviceInfo)
{
    QString certDeviceId = socket->peerCertificate().subjectDisplayName();
    DBusHelper::filterNonExportableCharacters(certDeviceId);
    if (deviceInfo.id != certDeviceId) {
        socket->disconnectFromHost();
        qCWarning(KDECONNECT_CORE) << "DeviceID in cert doesn't match deviceID in identity packet. " << deviceInfo.id << " vs " << certDeviceId;
        return;
    }

    // Socket disconnection will now be handled by LanDeviceLink
    disconnect(socket, &QAbstractSocket::disconnected, socket, &QObject::deleteLater);

    LanDeviceLink *deviceLink;
    // Do we have a link for this device already?
    QMap<QString, LanDeviceLink *>::iterator linkIterator = m_links.find(deviceInfo.id);
    if (linkIterator != m_links.end()) {
        deviceLink = linkIterator.value();
        if (deviceLink->deviceInfo().certificate != deviceInfo.certificate) {
            qWarning() << "LanLink was asked to replace a socket but the certificate doesn't match, aborting";
            return;
        }
        // qCDebug(KDECONNECT_CORE) << "Reusing link to" << deviceId;
        deviceLink->reset(socket);
    } else {
        deviceLink = new LanDeviceLink(deviceInfo, this, socket);
        // Socket disconnection will now be handled by LanDeviceLink
        disconnect(socket, &QAbstractSocket::disconnected, socket, &QObject::deleteLater);
        bool isDeviceTrusted = KdeConnectConfig::instance().trustedDevices().contains(deviceInfo.id);
        if (!isDeviceTrusted && m_links.size() > MAX_UNPAIRED_CONNECTIONS) {
            qCWarning(KDECONNECT_CORE) << "Too many unpaired devices to remember them all. Ignoring " << deviceInfo.id;
            socket->disconnectFromHost();
            socket->deleteLater();
            return;
        }
        m_links[deviceInfo.id] = deviceLink;
    }
    Q_EMIT onConnectionReceived(deviceLink);
}

#include "moc_lanlinkprovider.cpp"
