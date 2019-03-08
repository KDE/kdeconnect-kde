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

#include "bluetoothdevicelink.h"

#include "../linkprovider.h"
#include "bluetoothlinkprovider.h"
#include "bluetoothuploadjob.h"
#include "bluetoothdownloadjob.h"
#include "core_debug.h"
#include "connectionmultiplexer.h"
#include "multiplexchannel.h"

BluetoothDeviceLink::BluetoothDeviceLink(const QString& deviceId, LinkProvider* parent, ConnectionMultiplexer* connection, QSharedPointer<MultiplexChannel> socket)
    : DeviceLink(deviceId, parent)
    , mSocketReader(new DeviceLineReader(socket.data(), this))
    , mConnection(connection)
    , mChannel(socket)
    , mPairingHandler(new BluetoothPairingHandler(this))
{
    connect(mSocketReader, &DeviceLineReader::readyRead, this, &BluetoothDeviceLink::dataReceived);

    //We take ownership of the connection.
    //When the link provider destroys us,
    //the socket (and the reader) will be
    //destroyed as well
    mConnection->setParent(this);
    connect(socket.data(), &MultiplexChannel::aboutToClose, this, &QObject::deleteLater);
}

QString BluetoothDeviceLink::name()
{
    return QStringLiteral("BluetoothLink"); // Should be same in both android and kde version
}

bool BluetoothDeviceLink::sendPacket(NetworkPacket& np)
{
    if (np.hasPayload()) {
        BluetoothUploadJob* uploadJob = new BluetoothUploadJob(np.payload(), mConnection, this);
        np.setPayloadTransferInfo(uploadJob->transferInfo());
        uploadJob->start();
    }
    //TODO: handle too-big packets
    int written = mSocketReader->write(np.serialize());
    return (written != -1);
}

void BluetoothDeviceLink::userRequestsPair() {
    mPairingHandler->requestPairing();
}

void BluetoothDeviceLink::userRequestsUnpair() {
    mPairingHandler->unpair();
}

bool BluetoothDeviceLink::linkShouldBeKeptAlive() {
    return pairStatus() == Paired;
}

void BluetoothDeviceLink::dataReceived()
{
    if (mSocketReader->bytesAvailable() == 0) return;

    const QByteArray serializedPacket = mSocketReader->readLine();

    //qCDebug(KDECONNECT_CORE) << "BluetoothDeviceLink dataReceived" << packet;

    NetworkPacket packet((QString()));
    NetworkPacket::unserialize(serializedPacket, &packet);

    if (packet.type() == PACKET_TYPE_PAIR) {
        //TODO: Handle pair/unpair requests and forward them (to the pairing handler?)
        mPairingHandler->packetReceived(packet);
        return;
    }

    if (packet.hasPayloadTransferInfo()) {
        BluetoothDownloadJob* downloadJob = new BluetoothDownloadJob(mConnection, packet.payloadTransferInfo(), this);
        downloadJob->start();
        packet.setPayload(downloadJob->payload(), packet.payloadSize());
    }

    Q_EMIT receivedPacket(packet);

    if (mSocketReader->bytesAvailable() > 0) {
        QMetaObject::invokeMethod(this, &BluetoothDeviceLink::dataReceived, Qt::QueuedConnection);
    }
}
