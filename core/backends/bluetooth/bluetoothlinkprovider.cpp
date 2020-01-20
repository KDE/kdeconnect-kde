/**
 * Copyright 2016 Saikrishna Arcot <saiarcot895@gmail.com>
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

#include "bluetoothlinkprovider.h"
#include "core_debug.h"
#include "connectionmultiplexer.h"
#include "multiplexchannel.h"
#include "bluetoothdevicelink.h"

#include <QBluetoothServiceInfo>

BluetoothLinkProvider::BluetoothLinkProvider()
{
    mServiceUuid = QBluetoothUuid(QStringLiteral("185f3df4-3268-4e3f-9fca-d4d5059915bd"));

    connectTimer = new QTimer(this);
    connectTimer->setInterval(30000);
    connectTimer->setSingleShot(false);

    mServiceDiscoveryAgent = new QBluetoothServiceDiscoveryAgent(this);
    mServiceDiscoveryAgent->setUuidFilter(mServiceUuid);
    connect(connectTimer, &QTimer::timeout, this, [this]() {
        mServiceDiscoveryAgent->start();
    });

    connect(mServiceDiscoveryAgent, &QBluetoothServiceDiscoveryAgent::serviceDiscovered, this, &BluetoothLinkProvider::serviceDiscovered);
}

void BluetoothLinkProvider::onStart()
{
    QBluetoothLocalDevice localDevice;
    if (!localDevice.isValid()) {
        qCWarning(KDECONNECT_CORE) << "No local bluetooth adapter found";
        return;
    }

    mBluetoothServer = new QBluetoothServer(QBluetoothServiceInfo::RfcommProtocol, this);
    mBluetoothServer->setSecurityFlags(QBluetooth::Encryption | QBluetooth::Secure);
    connect(mBluetoothServer, SIGNAL(newConnection()), this, SLOT(serverNewConnection()));

    mServiceDiscoveryAgent->start();

    connectTimer->start();
    mKdeconnectService = mBluetoothServer->listen(mServiceUuid, QStringLiteral("KDE Connect"));
}

void BluetoothLinkProvider::onStop()
{
    if (!mBluetoothServer) {
        return;
    }

    connectTimer->stop();

    mKdeconnectService.unregisterService();
    mBluetoothServer->close();
    mBluetoothServer->deleteLater();
}

//I'm in a new network, let's be polite and introduce myself
void BluetoothLinkProvider::onNetworkChange()
{
}

void BluetoothLinkProvider::connectError()
{
    QBluetoothSocket* socket = qobject_cast<QBluetoothSocket*>(sender());
    if (!socket) return;

    qCWarning(KDECONNECT_CORE) << "Couldn't connect to socket:" << socket->errorString();

    disconnect(socket, SIGNAL(connected()), this, SLOT(clientConnected()));
    disconnect(socket, SIGNAL(readyRead()), this, SLOT(serverDataReceived()));
    disconnect(socket, SIGNAL(error(QBluetoothSocket::SocketError)), this, SLOT(connectError()));

    mSockets.remove(socket->peerAddress());
    socket->deleteLater();
}

void BluetoothLinkProvider::addLink(BluetoothDeviceLink* deviceLink, const QString& deviceId)
{
    QMap< QString, DeviceLink* >::iterator oldLinkIterator = mLinks.find(deviceId);
    if (oldLinkIterator != mLinks.end()) {
        DeviceLink* oldLink = oldLinkIterator.value();
        disconnect(oldLink, SIGNAL(destroyed(QObject*)),
                    this, SLOT(deviceLinkDestroyed(QObject*)));
        oldLink->deleteLater();
        mLinks.erase(oldLinkIterator);
    }

    mLinks[deviceId] = deviceLink;
}

void BluetoothLinkProvider::serviceDiscovered(const QBluetoothServiceInfo& old_info) {
    auto info = old_info;
    info.setServiceUuid(mServiceUuid);
    if (mSockets.contains(info.device().address())) {
        return;
    }

    QBluetoothSocket* socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol);

    //Delay before sending data
    QPointer<QBluetoothSocket> deleteableSocket = socket;
    connect(socket, &QBluetoothSocket::connected, this, [this,deleteableSocket]() {
        QTimer::singleShot(500, this, [this,deleteableSocket]() {
            clientConnected(deleteableSocket);
        });
    });
    connect(socket, SIGNAL(error(QBluetoothSocket::SocketError)), this, SLOT(connectError()));

    socket->connectToService(info);

    qCDebug(KDECONNECT_CORE()) << "Connecting to" << info.device().address();

    if (socket->error() != QBluetoothSocket::NoSocketError) {
        qCWarning(KDECONNECT_CORE) << "Socket connection error:" << socket->errorString();
    }
}

//I'm the new device and I'm connected to the existing device. Time to get data.
void BluetoothLinkProvider::clientConnected(QPointer<QBluetoothSocket> socket)
{
    if (!socket) return;

    auto peer = socket->peerAddress();

    qCDebug(KDECONNECT_CORE()) << "Connected to" << peer;

    if (mSockets.contains(socket->peerAddress())) {
        qCWarning(KDECONNECT_CORE()) << "Duplicate connection to" << peer;
        socket->close();
        socket->deleteLater();
        return;
    }

    ConnectionMultiplexer *multiplexer = new ConnectionMultiplexer(socket, this);

    mSockets.insert(peer, multiplexer);
    disconnect(socket, nullptr, this, nullptr);

    auto channel = QSharedPointer<MultiplexChannel>{multiplexer->getDefaultChannel().release()};
    connect(channel.data(), &MultiplexChannel::readyRead, this, [this,peer,channel] () { clientIdentityReceived(peer, channel); });
    connect(channel.data(), &MultiplexChannel::aboutToClose, this, [this,peer,channel] () { socketDisconnected(peer, channel.data()); });

    if (channel->bytesAvailable()) clientIdentityReceived(peer, channel);
    if (!channel->isOpen()) socketDisconnected(peer, channel.data());
}

//I'm the new device and the existing device sent me data.
void BluetoothLinkProvider::clientIdentityReceived(const QBluetoothAddress &peer, QSharedPointer<MultiplexChannel> socket)
{
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

    if (!success || receivedPacket.type() != PACKET_TYPE_IDENTITY) {
        qCWarning(KDECONNECT_CORE) << "Not an identity packet";
        mSockets.remove(peer);
        socket->close();
        socket->deleteLater();
        return;
    }

    qCDebug(KDECONNECT_CORE()) << "Received identity packet from" << peer;

    //TODO?
    //disconnect(socket, SIGNAL(error(QBluetoothSocket::SocketError)), this, SLOT(connectError()));

    const QString& deviceId = receivedPacket.get<QString>(QStringLiteral("deviceId"));
    BluetoothDeviceLink* deviceLink = new BluetoothDeviceLink(deviceId, this, mSockets[peer], socket);

    NetworkPacket np2;
    NetworkPacket::createIdentityPacket(&np2);
    success = deviceLink->sendPacket(np2);

    if (success) {

        qCDebug(KDECONNECT_CORE) << "Handshaking done (I'm the new device)";

        connect(deviceLink, SIGNAL(destroyed(QObject*)),
                this, SLOT(deviceLinkDestroyed(QObject*)));

        Q_EMIT onConnectionReceived(receivedPacket, deviceLink);

        //We kill any possible link from this same device
        addLink(deviceLink, deviceId);

    } else {
        // Connection might be lost. Delete it.
        delete deviceLink;
    }

    //We don't delete the socket because now it's owned by the BluetoothDeviceLink
}

//I'm the existing device, a new device is kindly introducing itself.
void BluetoothLinkProvider::serverNewConnection()
{
    QBluetoothSocket* socket = mBluetoothServer->nextPendingConnection();

    qCDebug(KDECONNECT_CORE()) << "Received connection from" << socket->peerAddress();

    QBluetoothAddress peer = socket->peerAddress();

    if (mSockets.contains(peer)) {
        qCDebug(KDECONNECT_CORE()) << "Duplicate connection from" << peer;
        socket->close();
        socket->deleteLater();
        return;
    }

    ConnectionMultiplexer *multiplexer = new ConnectionMultiplexer(socket, this);

    mSockets.insert(peer, multiplexer);
    disconnect(socket, nullptr, this, nullptr);

    auto channel = QSharedPointer<MultiplexChannel>{multiplexer->getDefaultChannel().release()};
    connect(channel.data(), &MultiplexChannel::readyRead, this, [this,peer,channel] () { serverDataReceived(peer, channel); });
    connect(channel.data(), &MultiplexChannel::aboutToClose, this, [this,peer,channel] () { socketDisconnected(peer, channel.data()); });

    if (!channel->isOpen()) {
        socketDisconnected(peer, channel.data());
        return;
    }

    NetworkPacket np2;
    NetworkPacket::createIdentityPacket(&np2);
    socket->write(np2.serialize());

    qCDebug(KDECONNECT_CORE()) << "Sent identity packet to" << socket->peerAddress();
}

//I'm the existing device and this is the answer to my identity packet (data received)
void BluetoothLinkProvider::serverDataReceived(const QBluetoothAddress &peer, QSharedPointer<MultiplexChannel> socket)
{
    QByteArray identityArray;
    socket->startTransaction();
    identityArray = socket->readLine();

    if (identityArray.isEmpty()) {
        socket->rollbackTransaction();
        return;
    }
    socket->commitTransaction();

    disconnect(socket.data(), &MultiplexChannel::readyRead, this, nullptr);

    NetworkPacket receivedPacket;
    bool success = NetworkPacket::unserialize(identityArray, &receivedPacket);

    if (!success || receivedPacket.type() != PACKET_TYPE_IDENTITY) {
        qCWarning(KDECONNECT_CORE) << "Not an identity packet.";
        mSockets.remove(peer);
        socket->close();
        socket->deleteLater();
        return;
    }

    qCDebug(KDECONNECT_CORE()) << "Received identity packet from" << peer;

    const QString& deviceId = receivedPacket.get<QString>(QStringLiteral("deviceId"));
    BluetoothDeviceLink* deviceLink = new BluetoothDeviceLink(deviceId, this, mSockets[peer], socket);

    connect(deviceLink, SIGNAL(destroyed(QObject*)),
            this, SLOT(deviceLinkDestroyed(QObject*)));

    Q_EMIT onConnectionReceived(receivedPacket, deviceLink);

    addLink(deviceLink, deviceId);
}

void BluetoothLinkProvider::deviceLinkDestroyed(QObject* destroyedDeviceLink)
{
    const QString id = destroyedDeviceLink->property("deviceId").toString();
    qCDebug(KDECONNECT_CORE()) << "Device disconnected:" << id;
    QMap< QString, DeviceLink* >::iterator oldLinkIterator = mLinks.find(id);
    if (oldLinkIterator != mLinks.end() && oldLinkIterator.value() == destroyedDeviceLink) {
        mLinks.erase(oldLinkIterator);
    }
}

void BluetoothLinkProvider::socketDisconnected(const QBluetoothAddress &peer, MultiplexChannel *socket)
{
    qCDebug(KDECONNECT_CORE()) << "Disconnected";
    disconnect(socket, nullptr, this, nullptr);

    mSockets.remove(peer);
}

BluetoothLinkProvider::~BluetoothLinkProvider()
{

}
