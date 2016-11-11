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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "bluetoothdevicelink.h"

#include "../linkprovider.h"
#include "bluetoothlinkprovider.h"
#include "bluetoothuploadjob.h"
#include "bluetoothdownloadjob.h"
#include "core_debug.h"

BluetoothDeviceLink::BluetoothDeviceLink(const QString& deviceId, LinkProvider* parent, QBluetoothSocket* socket)
    : DeviceLink(deviceId, parent)
    , mSocketReader(new DeviceLineReader(socket, this))
    , mBluetoothSocket(socket)
    , mPairingHandler(new BluetoothPairingHandler(this))
{
    connect(mSocketReader, SIGNAL(readyRead()),
            this, SLOT(dataReceived()));

    //We take ownership of the socket.
    //When the link provider destroys us,
    //the socket (and the reader) will be
    //destroyed as well
    mBluetoothSocket->setParent(this);
    connect(mBluetoothSocket, SIGNAL(disconnected()), this, SLOT(deleteLater()));
}

QString BluetoothDeviceLink::name()
{
    return "BluetoothLink"; // Should be same in both android and kde version
}

bool BluetoothDeviceLink::sendPackage(NetworkPackage& np)
{
    if (np.hasPayload()) {
        qCWarning(KDECONNECT_CORE) << "Sending packages with payload over bluetooth not yet supported";
        /*
        BluetoothUploadJob* uploadJob = new BluetoothUploadJob(np.payload(), mBluetoothSocket->peerAddress(), this);
        np.setPayloadTransferInfo(uploadJob->transferInfo());
        uploadJob->start();
        */
    }
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

    const QByteArray serializedPackage = mSocketReader->readLine();

    //qCDebug(KDECONNECT_CORE) << "BluetoothDeviceLink dataReceived" << package;

    NetworkPackage package(QString::null);
    NetworkPackage::unserialize(serializedPackage, &package);

    if (package.type() == PACKAGE_TYPE_PAIR) {
        //TODO: Handle pair/unpair requests and forward them (to the pairing handler?)
        mPairingHandler->packageReceived(package);
        return;
    }

    if (package.hasPayloadTransferInfo()) {
        BluetoothDownloadJob* downloadJob = new BluetoothDownloadJob(mBluetoothSocket->peerAddress(),
                                                                     package.payloadTransferInfo(), this);
        downloadJob->start();
        package.setPayload(downloadJob->payload(), package.payloadSize());
    }

    Q_EMIT receivedPackage(package);

    if (mSocketReader->bytesAvailable() > 0) {
        QMetaObject::invokeMethod(this, "dataReceived", Qt::QueuedConnection);
    }
}
