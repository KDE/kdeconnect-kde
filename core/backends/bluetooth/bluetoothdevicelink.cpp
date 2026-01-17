/**
 * SPDX-FileCopyrightText: 2016 Saikrishna Arcot <saiarcot895@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "bluetoothdevicelink.h"

#include "../linkprovider.h"
#include "bluetoothdownloadjob.h"
#include "bluetoothlinkprovider.h"
#include "bluetoothuploadjob.h"
#include "connectionmultiplexer.h"
#include "core_debug.h"
#include "multiplexchannel.h"

BluetoothDeviceLink::BluetoothDeviceLink(const DeviceInfo &deviceInfo,
                                         BluetoothLinkProvider *linkProvider,
                                         ConnectionMultiplexer *connection,
                                         QSharedPointer<MultiplexChannel> socket)
    : DeviceLink(linkProvider)
    , mConnection(connection)
    , mChannel(socket)
    , mDeviceInfo(deviceInfo)
{
    connect(socket.data(), &QIODevice::readyRead, this, &BluetoothDeviceLink::dataReceived);

    // We take ownership of the connection.
    // When the link provider destroys us,
    // the socket (and the reader) will be
    // destroyed as well
    mConnection->setParent(this);
    connect(socket.data(), &MultiplexChannel::aboutToClose, this, &QObject::deleteLater);
}

QString BluetoothDeviceLink::address() const
{
    return mConnection->address().toString();
}

bool BluetoothDeviceLink::sendPacket(NetworkPacket &np)
{
    if (np.hasPayload()) {
        BluetoothUploadJob *uploadJob = new BluetoothUploadJob(np.payload(), mConnection, this);
        np.setPayloadTransferInfo(uploadJob->transferInfo());
        uploadJob->start();
    }
    // TODO: handle too-big packets
    int written = mChannel->write(np.serialize());
    return (written != -1);
}

void BluetoothDeviceLink::dataReceived()
{
    while (mChannel->canReadLine()) {
        const QByteArray serializedPacket = mChannel->readLine();

        // qCDebug(KDECONNECT_CORE) << "BluetoothDeviceLink dataReceived" << packet;

        NetworkPacket packet;
        NetworkPacket::unserialize(serializedPacket, &packet);

        if (packet.hasPayloadTransferInfo()) {
            BluetoothDownloadJob *downloadJob = new BluetoothDownloadJob(mConnection, packet.payloadTransferInfo(), this);
            downloadJob->start();
            packet.setPayload(downloadJob->payload(), packet.payloadSize());
        }

        Q_EMIT receivedPacket(packet);
    }
}

#include "moc_bluetoothdevicelink.cpp"
