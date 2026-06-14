/**
 * SPDX-FileCopyrightText: 2016 Saikrishna Arcot <saiarcot895@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "bluetoothlinkproviderimpl.h"

#include <QBluetoothServiceInfo>

#include "bluetoothdevicelink.h"
#include "bluetoothlinkprovider.h"
#include "connectionmultiplexer.h"
#include "core_debug.h"
#include "kdeconnectconfig.h"
#include "multiplexchannel.h"

BluetoothLinkProviderImpl::BluetoothLinkProviderImpl(bool isDisabled, BluetoothLinkProvider *parent)
    : QObject(parent)
    , mServiceUuid(QBluetoothUuid(QStringLiteral("185f3df4-3268-4e3f-9fca-d4d5059915bd")))
    , mServiceDiscoveryAgent(new QBluetoothServiceDiscoveryAgent(this))
    , mDisabled(isDisabled)
    , mParent(parent)
{
    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl::ctor executed";

    mServiceDiscoveryAgent->setUuidFilter(mServiceUuid);
    connect(mServiceDiscoveryAgent, &QBluetoothServiceDiscoveryAgent::serviceDiscovered, this, &BluetoothLinkProviderImpl::serviceDiscovered);

    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl::ctor finished";
}

void BluetoothLinkProviderImpl::onStart()
{
    if (!mDisabled) {
        qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl::onStart executed";
        tryToInitialise();
    }
}

void BluetoothLinkProviderImpl::onStartDiscovery()
{
    if (!mDisabled && mServiceDiscoveryAgent != nullptr) {
        qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl::onStartDiscovery executed";
        mServiceDiscoveryAgent->start();
    }
}

void BluetoothLinkProviderImpl::tryToInitialise()
{
    QBluetoothLocalDevice localDevice;
    if (!localDevice.isValid()) {
        qCWarning(KDECONNECT_CORE) << "No local bluetooth adapter found";
        return;
    }
    if (!mBluetoothServer) {
        qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl::tryToInitialise re-setting up mBluetoothServer";

        mBluetoothServer = new QBluetoothServer(QBluetoothServiceInfo::RfcommProtocol, this);
        qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl::tryToInitialise new mBluetoothServer completed";

        mBluetoothServer->setSecurityFlags(QBluetooth::Security::Encryption | QBluetooth::Security::Secure);
        connect(mBluetoothServer, &QBluetoothServer::newConnection, this, &BluetoothLinkProviderImpl::serverNewConnection);

        qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl::tryToInitialise About to start server listen";
        mKdeconnectService = mBluetoothServer->listen(mServiceUuid, QStringLiteral("KDE Connect"));
    }
}

void BluetoothLinkProviderImpl::onStop()
{
    if (!mDisabled) {
        qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl::onStop executed";
        if (!mBluetoothServer) {
            return;
        }

        mKdeconnectService.unregisterService();
        mBluetoothServer->close();
        mBluetoothServer->deleteLater();
    }
}

void BluetoothLinkProviderImpl::enable()
{
    if (mDisabled) {
        mDisabled = false;
        tryToInitialise();
    }
}

void BluetoothLinkProviderImpl::disable()
{
    if (!mDisabled) {
        onStop();
        mDisabled = true;
        mBluetoothServer = nullptr;

        mServiceDiscoveryAgent->stop();
    }
}

void BluetoothLinkProviderImpl::onNetworkChange()
{
    if (!mDisabled) {
        qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl::onNetworkChange executed";
        tryToInitialise();
    }
}

void BluetoothLinkProviderImpl::connectError()
{
    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl::connectError executed";
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

void BluetoothLinkProviderImpl::addLink(BluetoothDeviceLink *deviceLink, const QString &deviceId)
{
    Q_EMIT onConnectionReceived(deviceLink);

    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl::addLink executed";
    if (mLinks.contains(deviceId)) {
        delete mLinks.take(deviceId); // not calling deleteLater because this triggers onLinkDestroyed
    }
    mLinks[deviceId] = deviceLink;
}

void BluetoothLinkProviderImpl::serviceDiscovered(const QBluetoothServiceInfo &old_info)
{
    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl::serviceDiscovered executed ";

    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl::serviceDiscovered info: " << old_info.device().address() << old_info.serviceName()
                             << old_info.serviceDescription() << old_info.socketProtocol() << old_info.isValid() << old_info.isComplete()
                             << old_info.isRegistered() << old_info.serviceClassUuids();

    auto info = *(new QBluetoothServiceInfo(old_info));
    info.setServiceUuid(mServiceUuid);
    if (mSockets.contains(info.device().address())) {
        qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl::serviceDiscovered sockets contains address, returning";
        return;
    }

    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl::serviceDiscovered before creating socket";
    QBluetoothSocket *socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol);

    // Delay before sending data
    QPointer<QBluetoothSocket> deleteableSocket = socket;
    connect(socket, &QBluetoothSocket::connected, this, [this, deleteableSocket]() {
        QTimer::singleShot(500, this, [this, deleteableSocket]() {
            qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl::serviceDiscovered after delay, executing clientConnected";
            clientConnected(deleteableSocket);
        });
    });

    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl::serviceDiscovered about to call connect";
    connect(socket, QOverload<QBluetoothSocket::SocketError>::of(&QBluetoothSocket::errorOccurred), this, &BluetoothLinkProviderImpl::connectError);

    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl::serviceDiscovered about to call connectToService";

    socket->connectToService(info);

    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl Connecting to" << info.device().address();

    if (socket->error() != QBluetoothSocket::SocketError::NoSocketError) {
        qCWarning(KDECONNECT_CORE) << "BluetoothLinkProviderImpl Socket connection error:" << socket->errorString();
    }
}

// I'm the new device and I'm connected to the existing device. Time to get data.
void BluetoothLinkProviderImpl::clientConnected(QPointer<QBluetoothSocket> socket)
{
    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl::clientConnected executed";
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
void BluetoothLinkProviderImpl::clientIdentityReceived(const QBluetoothAddress &peer, QSharedPointer<MultiplexChannel> socket)
{
    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl::clientIdentityReceived executed";
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
        qCWarning(KDECONNECT_CORE) << "BluetoothLinkProviderImpl: Invalid identity packet received";
        mSockets.remove(peer);
        socket->close();
        socket->deleteLater();
        return;
    }

    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl Received identity packet from" << peer;

    QString certificateBody = receivedPacket.get<QString>(QStringLiteral("certificate"));
    if (certificateBody.startsWith(QStringLiteral("-----BEGIN CERTIFICATE-----")) == false) {
        qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl clientIdentityReceived Fixing PEM encoding";
        certificateBody = certificateBody.prepend(QStringLiteral("-----BEGIN CERTIFICATE-----\n")).append(QStringLiteral("\n-----END CERTIFICATE-----\n"));
    }
    QSslCertificate receivedCertificate(certificateBody.toLatin1());
    DeviceInfo deviceInfo = DeviceInfo::FromIdentityPacketAndCert(receivedPacket, receivedCertificate);
    BluetoothDeviceLink *deviceLink = new BluetoothDeviceLink(deviceInfo, mParent, mSockets[peer], socket);

    DeviceInfo myDeviceInfo = KdeConnectConfig::instance().deviceInfo();
    NetworkPacket myIdentity = myDeviceInfo.toIdentityPacket();
    myIdentity.set(QStringLiteral("certificate"), QString::fromLatin1(myDeviceInfo.certificate.toPem()));
    success = deviceLink->sendPacket(myIdentity);

    if (success) {
        qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl Handshaking done (I'm the new device)";

        // We kill any possible link from this same device
        addLink(deviceLink, deviceInfo.id);

    } else {
        // Connection might be lost. Delete it.
        delete deviceLink;
    }

    // We don't delete the socket because now it's owned by the BluetoothDeviceLink
}

// I'm the existing device, a new device is kindly introducing itself.
void BluetoothLinkProviderImpl::serverNewConnection()
{
    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl::serverNewConnection executed";
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
void BluetoothLinkProviderImpl::serverDataReceived(const QBluetoothAddress &peer, QSharedPointer<MultiplexChannel> socket)
{
    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl::serverDataReceived executed";
    QByteArray identityArray;
    if (!socket) {
        qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl::serverDataReceived but socket no longer exists";
        return;
    }

    socket->startTransaction();
    if (socket->isReadable() == false || socket->canReadLine() == false) {
        qCWarning(KDECONNECT_CORE) << "BluetoothLinkProviderImpl::serverDataReceived socket is not readable, bailing out";
        socket->rollbackTransaction();
        return;
    }
    identityArray = socket->readLine();

    if (identityArray.isEmpty()) {
        qCWarning(KDECONNECT_CORE) << "BluetoothLinkProviderImpl::serverDataReceived identity array is empty, bailing out";
        socket->rollbackTransaction();
        return;
    }
    socket->commitTransaction();

    disconnect(socket.data(), &MultiplexChannel::readyRead, this, nullptr);

    NetworkPacket receivedPacket;
    bool success = NetworkPacket::unserialize(identityArray, &receivedPacket);

    if (!success || !DeviceInfo::isValidIdentityPacket(&receivedPacket)) {
        qCWarning(KDECONNECT_CORE) << "BluetoothLinkProviderImpl: Invalid identity packet received";
        mSockets.remove(peer);
        socket->close();
        socket->deleteLater();
        return;
    }

    qCDebug(KDECONNECT_CORE) << "Received identity packet from" << peer;

    QString certificateBody = receivedPacket.get<QString>(QStringLiteral("certificate"));
    if (certificateBody.startsWith(QStringLiteral("-----BEGIN CERTIFICATE-----")) == false) {
        qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl serverDataReceived remoteDeviceInfo fixing PEM encoding";
        certificateBody = certificateBody.prepend(QStringLiteral("-----BEGIN CERTIFICATE-----\n")).append(QStringLiteral("-----END CERTIFICATE-----\n"));
    }
    QSslCertificate receivedCertificate(certificateBody.toLatin1());
    DeviceInfo deviceInfo = DeviceInfo::FromIdentityPacketAndCert(receivedPacket, receivedCertificate);
    BluetoothDeviceLink *deviceLink = new BluetoothDeviceLink(deviceInfo, mParent, mSockets[peer], socket);

    addLink(deviceLink, deviceInfo.id);
}

void BluetoothLinkProviderImpl::onLinkDestroyed(const QString &deviceId, DeviceLink *oldPtr)
{
    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl deviceLinkDestroyed" << deviceId;
    DeviceLink *link = mLinks.take(deviceId);
    Q_ASSERT(link == oldPtr);
}

void BluetoothLinkProviderImpl::socketDisconnected(const QBluetoothAddress &peer, MultiplexChannel *socket)
{
    qCDebug(KDECONNECT_CORE) << "BluetoothLinkProviderImpl socketDisconnected";
    disconnect(socket, nullptr, this, nullptr);
    qCDebug(KDECONNECT_CORE) << "socketDisconnected :: After calling disconnect";

    mSockets.remove(peer);
    qCDebug(KDECONNECT_CORE) << "socketDisconnected :: After calling remove";
}

BluetoothLinkProviderImpl::~BluetoothLinkProviderImpl()
{
}

#include "moc_bluetoothlinkproviderimpl.cpp"
