/**
 * SPDX-FileCopyrightText: 2016 Saikrishna Arcot <saiarcot895@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "bluetoothlinkprovider.h"
#include "bluetoothdevicelink.h"
#include "connectionmultiplexer.h"
#include "core_debug.h"
#include "kdeconnectconfig.h"
#include "multiplexchannel.h"

#include <QBluetoothServiceInfo>

BluetoothLinkProvider::BluetoothLinkProvider(bool isDisabled)
    : mServiceUuid(QBluetoothUuid(QStringLiteral("185f3df4-3268-4e3f-9fca-d4d5059915bd")))
    , mServiceDiscoveryAgent(new QBluetoothServiceDiscoveryAgent(this))
    , mDisabled(isDisabled)
{
    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider::ctor executed";

    mServiceDiscoveryAgent->setUuidFilter(mServiceUuid);
    connect(mServiceDiscoveryAgent, &QBluetoothServiceDiscoveryAgent::serviceDiscovered, this, &BluetoothLinkProvider::serviceDiscovered);

    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider::ctor finished";
}

void BluetoothLinkProvider::onStart()
{
    if (!mDisabled) {
        qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider::onStart executed";
        tryToInitialise();
    }
}

void BluetoothLinkProvider::onStartDiscovery()
{
    if (!mDisabled && mServiceDiscoveryAgent != nullptr) {
        qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider::onStartDiscovery executed";
        mServiceDiscoveryAgent->start();
    }
}

void BluetoothLinkProvider::tryToInitialise()
{
    QBluetoothLocalDevice localDevice;
    if (!localDevice.isValid()) {
        qCWarning(KDECONNECT_CORE) << "No local bluetooth adapter found";
        return;
    }
    if (!mBluetoothServer) {
        qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider::tryToInitialise re-setting up mBluetoothServer";

        mBluetoothServer = new QBluetoothServer(QBluetoothServiceInfo::RfcommProtocol, this);
        qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider::tryToInitialise new mBluetoothServer completed";

        mBluetoothServer->setSecurityFlags(QBluetooth::Security::Encryption | QBluetooth::Security::Secure);
        connect(mBluetoothServer, &QBluetoothServer::newConnection, this, &BluetoothLinkProvider::serverNewConnection);

        qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider::tryToInitialise About to start server listen";
        mKdeconnectService = mBluetoothServer->listen(mServiceUuid, QStringLiteral("KDE Connect"));
    }
}

void BluetoothLinkProvider::onStop()
{
    if (!mDisabled) {
        qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider::onStop executed";
        if (!mBluetoothServer) {
            return;
        }

        mKdeconnectService.unregisterService();
        mBluetoothServer->close();
        mBluetoothServer->deleteLater();
    }
}

void BluetoothLinkProvider::enable()
{
    if (mDisabled) {
        mDisabled = false;
        tryToInitialise();
    }
}

void BluetoothLinkProvider::disable()
{
    if (!mDisabled) {
        mDisabled = true;
        onStop();

        mBluetoothServer = nullptr;
        mServiceDiscoveryAgent = nullptr;
    }
}

void BluetoothLinkProvider::onNetworkChange()
{
    if (!mDisabled) {
        qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider::onNetworkChange executed";
        tryToInitialise();
    }
}

void BluetoothLinkProvider::connectError()
{
    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider::connectError executed";
    QBluetoothSocket *socket = qobject_cast<QBluetoothSocket *>(sender());
    if (!socket)
        return;

    qCWarning(KDECONNECT_CORE) << "Couldn't connect to bluetooth socket:" << socket->errorString();

    disconnect(socket, &QBluetoothSocket::connected, this, nullptr);
    disconnect(socket, &QBluetoothSocket::readyRead, this, nullptr);
    disconnect(socket, QOverload<QBluetoothSocket::SocketError>::of(&QBluetoothSocket::errorOccurred), this, nullptr);

    mSockets.remove(socket->peerAddress());
    socket->deleteLater();
}

void BluetoothLinkProvider::addLink(BluetoothDeviceLink *deviceLink, const QString &deviceId)
{
    Q_EMIT onConnectionReceived(deviceLink);

    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider::addLink executed";
    if (mLinks.contains(deviceId)) {
        delete mLinks.take(deviceId); // not calling deleteLater because this triggers onLinkDestroyed
    }
    mLinks[deviceId] = deviceLink;
}

void BluetoothLinkProvider::serviceDiscovered(const QBluetoothServiceInfo &old_info)
{
    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider::serviceDiscovered executed ";

    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider::serviceDiscovered info: " << old_info.device().address() << old_info.serviceName()
                             << old_info.serviceDescription() << old_info.socketProtocol() << old_info.isValid() << old_info.isComplete()
                             << old_info.isRegistered() << old_info.serviceClassUuids();

    auto info = *(new QBluetoothServiceInfo(old_info));
    info.setServiceUuid(mServiceUuid);
    if (mSockets.contains(info.device().address())) {
        qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider::serviceDiscovered sockets contains address, returning";
        return;
    }

    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider::serviceDiscovered before creating socket";
    QBluetoothSocket *socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol);

    // Delay before sending data
    QPointer<QBluetoothSocket> deleteableSocket = socket;
    connect(socket, &QBluetoothSocket::connected, this, [this, deleteableSocket]() {
        QTimer::singleShot(500, this, [this, deleteableSocket]() {
            qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider::serviceDiscovered after delay, executing clientConnected";
            clientConnected(deleteableSocket);
        });
    });

    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider::serviceDiscovered about to call connect";
    connect(socket, QOverload<QBluetoothSocket::SocketError>::of(&QBluetoothSocket::errorOccurred), this, &BluetoothLinkProvider::connectError);

    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider::serviceDiscovered about to call connectToService";

    socket->connectToService(info);

    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider Connecting to" << info.device().address();

    if (socket->error() != QBluetoothSocket::SocketError::NoSocketError) {
        qCWarning(KDECONNECT_CORE) << "BluetoothLinkProvider Socket connection error:" << socket->errorString();
    }
}

// I'm the new device and I'm connected to the existing device. Time to get data.
void BluetoothLinkProvider::clientConnected(QPointer<QBluetoothSocket> socket)
{
    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider::clientConnected executed";
    if (!socket)
        return;

    auto peer = socket->peerAddress();

    qCDebug(KDECONNECT_CORE) << "Connected to" << peer;

    if (mSockets.contains(socket->peerAddress())) {
        qCWarning(KDECONNECT_CORE) << "Duplicate connection to" << peer;
        socket->close();
        socket->deleteLater();
        return;
    }

    ConnectionMultiplexer *multiplexer = new ConnectionMultiplexer(socket, this);

    mSockets.insert(peer, multiplexer);
    disconnect(socket, nullptr, this, nullptr);

    auto channel = QSharedPointer<MultiplexChannel>{multiplexer->getDefaultChannel().release()};
    connect(channel.data(), &MultiplexChannel::readyRead, this, [this, peer, channel]() {
        clientIdentityReceived(peer, channel);
    });
    connect(channel.data(), &MultiplexChannel::aboutToClose, this, [this, peer, channel]() {
        socketDisconnected(peer, channel.data());
    });

    if (channel->bytesAvailable())
        clientIdentityReceived(peer, channel);
    if (!channel->isOpen())
        socketDisconnected(peer, channel.data());
}

// I'm the new device and the existing device sent me data.
void BluetoothLinkProvider::clientIdentityReceived(const QBluetoothAddress &peer, QSharedPointer<MultiplexChannel> socket)
{
    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider::clientIdentityReceived executed";
    socket->startTransaction();

    QByteArray identityArray = socket->readLine();
    if (identityArray.isEmpty()) {
        socket->rollbackTransaction();
        return;
    }
    socket->commitTransaction();

    disconnect(socket.data(), &MultiplexChannel::readyRead, this, nullptr);

    NetworkPacket receivedPacket;
    bool success = NetworkPacket::unserialize(identityArray, &receivedPacket);

    if (!success || !DeviceInfo::isValidIdentityPacket(&receivedPacket)) {
        qCWarning(KDECONNECT_CORE) << "BluetoothLinkProvider: Invalid identity packet received";
        mSockets.remove(peer);
        socket->close();
        socket->deleteLater();
        return;
    }

    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider Received identity packet from" << peer;

    // TODO?
    // disconnect(socket, &QAbstractSocket::error, this, &BluetoothLinkProvider::connectError);

    QSslCertificate receivedCertificate(receivedPacket.get<QString>(QStringLiteral("certificate")).toLatin1());
    DeviceInfo deviceInfo = deviceInfo.FromIdentityPacketAndCert(receivedPacket, receivedCertificate);
    BluetoothDeviceLink *deviceLink = new BluetoothDeviceLink(deviceInfo, this, mSockets[peer], socket);

    DeviceInfo myDeviceInfo = KdeConnectConfig::instance().deviceInfo();
    NetworkPacket myIdentity = myDeviceInfo.toIdentityPacket();
    myIdentity.set(QStringLiteral("certificate"), QString::fromLatin1(myDeviceInfo.certificate.toPem()));
    success = deviceLink->sendPacket(myIdentity);

    if (success) {
        qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider Handshaking done (I'm the new device)";

        // We kill any possible link from this same device
        addLink(deviceLink, deviceInfo.id);

    } else {
        // Connection might be lost. Delete it.
        delete deviceLink;
    }

    // We don't delete the socket because now it's owned by the BluetoothDeviceLink
}

// I'm the existing device, a new device is kindly introducing itself.
void BluetoothLinkProvider::serverNewConnection()
{
    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider::serverNewConnection executed";
    QBluetoothSocket *socket = mBluetoothServer->nextPendingConnection();

    qCDebug(KDECONNECT_CORE) << socket->peerAddress() << "Received connection";

    QBluetoothAddress peer = socket->peerAddress();

    if (mSockets.contains(peer)) {
        qCDebug(KDECONNECT_CORE) << "Duplicate connection from" << peer;
        socket->close();
        socket->deleteLater();
        return;
    }

    ConnectionMultiplexer *multiplexer = new ConnectionMultiplexer(socket, this);

    qCDebug(KDECONNECT_CORE) << socket->peerAddress() << "Multiplexer Instantiated";

    mSockets.insert(peer, multiplexer);
    disconnect(socket, nullptr, this, nullptr);

    auto channel = QSharedPointer<MultiplexChannel>{multiplexer->getDefaultChannel().release()};
    connect(channel.data(), &MultiplexChannel::readyRead, this, [this, peer, channel]() {
        serverDataReceived(peer, channel);
    });
    connect(channel.data(), &MultiplexChannel::aboutToClose, this, [this, peer, channel]() {
        socketDisconnected(peer, channel.data());
    });

    if (!channel->isOpen()) {
        socketDisconnected(peer, channel.data());
        return;
    }

    qCDebug(KDECONNECT_CORE) << socket->peerAddress() << "Building and sending identity Packet ";

    DeviceInfo myDeviceInfo = KdeConnectConfig::instance().deviceInfo();
    NetworkPacket myIdentity = myDeviceInfo.toIdentityPacket();
    myIdentity.set(QStringLiteral("certificate"), QString::fromLatin1(myDeviceInfo.certificate.toPem()));
    auto identityPacket = myIdentity.serialize();

    channel->write(identityPacket);

    qCDebug(KDECONNECT_CORE) << "Sent identity packet on default channel to" << socket->peerAddress();
}

// I'm the existing device and this is the answer to my identity packet (data received)
void BluetoothLinkProvider::serverDataReceived(const QBluetoothAddress &peer, QSharedPointer<MultiplexChannel> socket)
{
    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider::serverDataReceived executed";
    QByteArray identityArray;
    if (!socket) {
        qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider::serverDataReceived but socket no longer exists";
        return;
    }

    socket->startTransaction();
    if (socket->isReadable() == false || socket->canReadLine() == false) {
        qCWarning(KDECONNECT_CORE) << "BluetoothLinkProvider::serverDataReceived socket is not readable, bailing out";
        socket->rollbackTransaction();
        return;
    }
    identityArray = socket->readLine();

    if (identityArray.isEmpty()) {
        qCWarning(KDECONNECT_CORE) << "BluetoothLinkProvider::serverDataReceived identity array is empty, bailing out";
        socket->rollbackTransaction();
        return;
    }
    socket->commitTransaction();

    disconnect(socket.data(), &MultiplexChannel::readyRead, this, nullptr);

    NetworkPacket receivedPacket;
    bool success = NetworkPacket::unserialize(identityArray, &receivedPacket);

    if (!success || !DeviceInfo::isValidIdentityPacket(&receivedPacket)) {
        qCWarning(KDECONNECT_CORE) << "BluetoothLinkProvider: Invalid identity packet received";
        mSockets.remove(peer);
        socket->close();
        socket->deleteLater();
        return;
    }

    qCDebug(KDECONNECT_CORE) << "Received identity packet from" << peer;

    QSslCertificate receivedCertificate(receivedPacket.get<QString>(QStringLiteral("certificate")).toLatin1());
    DeviceInfo deviceInfo = deviceInfo.FromIdentityPacketAndCert(receivedPacket, receivedCertificate);
    BluetoothDeviceLink *deviceLink = new BluetoothDeviceLink(deviceInfo, this, mSockets[peer], socket);

    addLink(deviceLink, deviceInfo.id);
}

void BluetoothLinkProvider::onLinkDestroyed(const QString &deviceId, DeviceLink *oldPtr)
{
    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider deviceLinkDestroyed" << deviceId;
    DeviceLink *link = mLinks.take(deviceId);
    Q_ASSERT(link == oldPtr);
}

void BluetoothLinkProvider::socketDisconnected(const QBluetoothAddress &peer, MultiplexChannel *socket)
{
    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProvider socketDisconnected";
    disconnect(socket, nullptr, this, nullptr);
    qCDebug(KDECONNECT_CORE) << "socketDisconnected :: After calling disconnect";

    mSockets.remove(peer);
    qCDebug(KDECONNECT_CORE) << "socketDisconnected :: After calling remove";
}

BluetoothLinkProvider::~BluetoothLinkProvider()
{
}

#include "moc_bluetoothlinkprovider.cpp"
