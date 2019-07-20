/**
 * Copyright 2019 Albert Vaca <albertvaka@gmail.com>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "mdnslinkprovider.h"
#include "core_debug.h"

#include <KDNSSD/DNSSD/PublicService>
#include <KDNSSD/DNSSD/ServiceBrowser>
#include <KDNSSD/DNSSD/RemoteService>

#include <QHostInfo>
#include <QTcpServer>
#include <QMetaEnum>
#include <QNetworkProxy>
#include <QNetworkSession>
#include <QNetworkConfigurationManager>
#include <QSslCipher>
#include <QSslConfiguration>

#include "daemon.h"
#include "landevicelink.h"
#include "lanpairinghandler.h"
#include "kdeconnectconfig.h"

MDNSLinkProvider::MDNSLinkProvider()
    : m_server(new Server(this))
    , m_tcpPort(0)
    , m_publisher(nullptr)
    , m_serviceBrowser(nullptr)
{

    m_server->setProxy(QNetworkProxy::NoProxy);
    connect(m_server, &QTcpServer::newConnection, this, &MDNSLinkProvider::newConnection);

    //Detect when a network interface changes status, so we announce ourselves in the new network
    QNetworkConfigurationManager* networkManager = new QNetworkConfigurationManager(this);
    connect(networkManager, &QNetworkConfigurationManager::configurationChanged, this, &MDNSLinkProvider::onNetworkConfigurationChanged);

}

#include <iostream>

void MDNSLinkProvider::initializeMDNS()
{
    KdeConnectConfig* config = KdeConnectConfig::instance();

    std::cout << "Init PublicService" << std::endl;

    // Restart the publishing.
    delete m_publisher;
    m_publisher = new KDNSSD::PublicService(QStringLiteral("KDEConnect on ") + config->name(), QStringLiteral("_kdeconnect._tcp"), m_tcpPort);

/*
    QMap<QString, QByteArray> data;
    data[QStringLiteral("protocolVersion")] = QString::number(NetworkPacket::s_protocolVersion).toUtf8();
    data[QStringLiteral("type")] = config->deviceType().toUtf8();
    data[QStringLiteral("name")] = config->name().toUtf8();
    data[QStringLiteral("id")] = config->deviceId().toUtf8();
    m_publisher->setTextData(data);
*/

    if (m_publisher->publish()) {
        qCDebug(KDECONNECT_CORE) << "mDNS: published successfully";
    } else {
        qCDebug(KDECONNECT_CORE) << "mDNS: failed to publish";
    }

    std::cout << "Init ServiceBrowser" << std::endl;

    delete m_serviceBrowser;
    m_serviceBrowser = new KDNSSD::ServiceBrowser(QStringLiteral("_kdeconnect._tcp"), true); //TODO: Check if autoresolve can be false to save bandwidth
    connect(m_serviceBrowser, &KDNSSD::ServiceBrowser::serviceAdded, [this](KDNSSD::RemoteService::Ptr service) {
        std::cout << service->hostName().toStdString() << service->serviceName().toStdString() << std::endl;
        QSslSocket* socket = new QSslSocket(this);
        connect(socket, &QAbstractSocket::connected, this, &MDNSLinkProvider::tcpSocketConnected);
        connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &MDNSLinkProvider::connectError);
        socket->setProxy(QNetworkProxy::NoProxy);
        socket->connectToHost(service->hostName(), service->port());
    });
    connect(m_serviceBrowser, &KDNSSD::ServiceBrowser::finished, []() {
        std::cout << "Finished discovery" << std::endl;
    });
    m_serviceBrowser->startBrowse();

    //Discovery
}


void MDNSLinkProvider::onNetworkConfigurationChanged(const QNetworkConfiguration& config)
{
    if (m_lastConfig != config && config.state() == QNetworkConfiguration::Active) {
        m_lastConfig = config;
        onNetworkChange();
    }
}

MDNSLinkProvider::~MDNSLinkProvider()
{
}

void MDNSLinkProvider::onStart()
{
    m_tcpPort = MIN_TCP_PORT;
    while (!m_server->listen(QHostAddress::Any, m_tcpPort)) {
        m_tcpPort++;
        if (m_tcpPort > MAX_TCP_PORT) { //No ports available?
            qCritical(KDECONNECT_CORE) << "Error opening a port in range" << MIN_TCP_PORT << "-" << MAX_TCP_PORT;
            m_tcpPort = 0;
            return;
        }
    }

    onNetworkChange();
    qCDebug(KDECONNECT_CORE) << "MDNSLinkProvider started";
}

void MDNSLinkProvider::onStop()
{
    m_server->close();
    qCDebug(KDECONNECT_CORE) << "MDNSLinkProvider stopped";
}

void MDNSLinkProvider::onNetworkChange()
{
    initializeMDNS();
}

void MDNSLinkProvider::connectError(QAbstractSocket::SocketError socketError)
{
    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) return;

    qCDebug(KDECONNECT_CORE) << "Socket error" << socketError;

    //The socket we created didn't work, and we didn't manage
    //to create a LanDeviceLink from it, deleting everything.
    delete m_receivedIdentityPackets.take(socket).np;
    delete socket;
}

//We received a UDP packet and answered by connecting to them by TCP. This gets called on a successful connection.
void MDNSLinkProvider::tcpSocketConnected()
{
    std::cout << "tcpSocketConnected" << std::endl;
    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) {
        return;
    }

    configureSocket(socket);

    connect(socket, &QAbstractSocket::disconnected, socket, &QObject::deleteLater);

    std::cout << "sending our identity packet to the socket we are connecting to" << std::endl;
    sendOurIdentity(socket);
    std::cout << "now receiving" << std::endl;
    NetworkPacket* receivedPacket = receiveTheirIdentity(socket);
    if (!receivedPacket) {
        qCWarning(KDECONNECT_CORE) << "Aborting tcpSocketConnected since we couldn't get their identity";
        return;
    }
    m_receivedIdentityPackets[socket].np = receivedPacket;
    const QString& deviceId = receivedPacket->get<QString>(QStringLiteral("deviceId"));


    qCDebug(KDECONNECT_CORE) << "TCP connection done (i'm the existing device)";

    bool isDeviceTrusted = KdeConnectConfig::instance()->trustedDevices().contains(deviceId);
    configureSslSocket(socket, deviceId, isDeviceTrusted);

    qCDebug(KDECONNECT_CORE) << "Starting server ssl (I'm the client TCP socket)";

    connect(socket, &QSslSocket::encrypted, this, &MDNSLinkProvider::encrypted);

    if (isDeviceTrusted) {
        connect(socket, QOverload<const QList<QSslError> &>::of(&QSslSocket::sslErrors), this, &MDNSLinkProvider::sslErrors);
    }

    socket->startServerEncryption();
}

void MDNSLinkProvider::encrypted()
{
    qCDebug(KDECONNECT_CORE) << "Socket successfully established an SSL connection";

    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) return;
    // TODO delete me?
    disconnect(socket, QOverload<const QList<QSslError> &>::of(&QSslSocket::sslErrors), this, &MDNSLinkProvider::sslErrors);

    Q_ASSERT(socket->mode() != QSslSocket::UnencryptedMode);
    LanDeviceLink::ConnectionStarted connectionOrigin = (socket->mode() == QSslSocket::SslClientMode)? LanDeviceLink::Locally : LanDeviceLink::Remotely;

    NetworkPacket* receivedPacket = m_receivedIdentityPackets[socket].np;
    const QString& deviceId = receivedPacket->get<QString>(QStringLiteral("deviceId"));

    addLink(deviceId, socket, receivedPacket, connectionOrigin);

    // Copied from tcpSocketConnected slot, now delete received packet
    delete m_receivedIdentityPackets.take(socket).np;
}

void MDNSLinkProvider::sslErrors(const QList<QSslError>& errors)
{
    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) return;

    qCDebug(KDECONNECT_CORE) << "Failing due to " << errors;
    Device* device = Daemon::instance()->getDevice(socket->peerVerifyName());
    if (device) {
        device->unpair();
    }

    delete m_receivedIdentityPackets.take(socket).np;
    // Socket disconnects itself on ssl error and will be deleted by deleteLater slot, no need to delete manually
}

//I'm the new device and this is the answer to my UDP identity packet (no data received yet). They are connecting to us through TCP, and they should send an identity.
void MDNSLinkProvider::newConnection()
{
    qCDebug(KDECONNECT_CORE) << "MDNSLinkProvider newConnection";

    while (m_server->hasPendingConnections()) {
        QSslSocket* socket = m_server->nextPendingConnection();
        configureSocket(socket);
        //This socket is still managed by us (and child of the QTcpServer), if
        //it disconnects before we manage to pass it to a LanDeviceLink, it's
        //our responsibility to delete it. We do so with this connection.
        connect(socket, &QAbstractSocket::disconnected, socket, &QObject::deleteLater);
        connect(socket, &QIODevice::readyRead, this, &MDNSLinkProvider::dataReceived);

    }
}

//I'm the new device and this is the answer to my UDP identity packet (data received)
void MDNSLinkProvider::dataReceived()
{
    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());
    socket->startTransaction();

    std::cout << "dataReceived" << std::endl;

    sendOurIdentity(socket);
    NetworkPacket* theirIdentity = receiveTheirIdentity(socket);
    if (!theirIdentity) {
        qCWarning(KDECONNECT_CORE) << "Aborting dataReceived since we couldn't get their identity";
        return;
    }
    m_receivedIdentityPackets[socket].np = theirIdentity;

    const QString& deviceId = theirIdentity->get<QString>(QStringLiteral("deviceId"));
    qCDebug(KDECONNECT_CORE) << "Handshaking done (i'm the new device)";

    //This socket will now be owned by the LanDeviceLink or we don't want more data to be received, forget about it
    disconnect(socket, &QIODevice::readyRead, this, &MDNSLinkProvider::dataReceived);

    bool isDeviceTrusted = KdeConnectConfig::instance()->trustedDevices().contains(deviceId);
    configureSslSocket(socket, deviceId, isDeviceTrusted);

    qCDebug(KDECONNECT_CORE) << "Starting client ssl (but I'm the server TCP socket)";

    connect(socket, &QSslSocket::encrypted, this, &MDNSLinkProvider::encrypted);

    if (isDeviceTrusted) {
        connect(socket, QOverload<const QList<QSslError> &>::of(&QSslSocket::sslErrors), this, &MDNSLinkProvider::sslErrors);
    }

    socket->startClientEncryption();

}

void MDNSLinkProvider::deviceLinkDestroyed(QObject* destroyedDeviceLink)
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

void MDNSLinkProvider::configureSslSocket(QSslSocket* socket, const QString& deviceId, bool isDeviceTrusted)
{
    /*
    // Setting supported ciphers manually, to match those on Android (FIXME: Test if this can be left unconfigured and still works for Android 4)
    QList<QSslCipher> socketCiphers;
    socketCiphers.append(QSslCipher(QStringLiteral("ECDHE-ECDSA-AES256-GCM-SHA384")));
    socketCiphers.append(QSslCipher(QStringLiteral("ECDHE-ECDSA-AES128-GCM-SHA256")));
    socketCiphers.append(QSslCipher(QStringLiteral("ECDHE-RSA-AES128-SHA")));
    // Configure for ssl
    QSslConfiguration sslConfig;
    sslConfig.setCiphers(socketCiphers);

    socket->setSslConfiguration(sslConfig);
    */
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
    //    Q_FOREACH (const QSslError& error, errors) {
    //        qCDebug(KDECONNECT_CORE) << "SSL Error:" << error.errorString();
    //    }
    //});
}

void MDNSLinkProvider::configureSocket(QSslSocket* socket) {

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

void MDNSLinkProvider::addLink(const QString& deviceId, QSslSocket* socket, NetworkPacket* receivedPacket, LanDeviceLink::ConnectionStarted connectionOrigin)
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
        connect(deviceLink, &QObject::destroyed, this, &MDNSLinkProvider::deviceLinkDestroyed);
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

LanPairingHandler* MDNSLinkProvider::createPairingHandler(DeviceLink* link)
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

void MDNSLinkProvider::userRequestsPair(const QString& deviceId)
{
    LanPairingHandler* ph = createPairingHandler(m_links.value(deviceId));
    ph->requestPairing();
}

void MDNSLinkProvider::userRequestsUnpair(const QString& deviceId)
{
    LanPairingHandler* ph = createPairingHandler(m_links.value(deviceId));
    ph->unpair();
}

void MDNSLinkProvider::incomingPairPacket(DeviceLink* deviceLink, const NetworkPacket& np)
{
    LanPairingHandler* ph = createPairingHandler(deviceLink);
    ph->packetReceived(np);
}


NetworkPacket* MDNSLinkProvider::receiveTheirIdentity(QSslSocket* socket)
{

    const QByteArray data = socket->readLine();
    qCDebug(KDECONNECT_CORE) << "MDNSLinkProvider received reply:" << data;

    NetworkPacket* theirIdentity = new NetworkPacket();
    bool success = NetworkPacket::unserialize(data, theirIdentity);

    if (!success) {
        delete theirIdentity;
        socket->rollbackTransaction();
        return nullptr;
    }
    socket->commitTransaction();

    if (theirIdentity->type() != PACKET_TYPE_IDENTITY) {
        qCWarning(KDECONNECT_CORE) << "MDNSLinkProvider/newConnection: Expected identity, received " << theirIdentity->type();
        delete theirIdentity;
        return nullptr;
    }

    // Needed in "encrypted" if ssl is used, similar to "tcpSocketConnected"
    return theirIdentity;
}

void MDNSLinkProvider::sendOurIdentity(QSslSocket* socket)
{
    NetworkPacket* ourIdentity = new NetworkPacket();
    NetworkPacket::createIdentityPacket(ourIdentity);
    socket->write(ourIdentity->serialize());
    delete ourIdentity;
}
