/**
 * Copyright 2015 Saikrishna Arcot <saiarcot895@gmail.com>
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

BluetoothDeviceLink::BluetoothDeviceLink(const QString& deviceId, LinkProvider* parent, QBluetoothSocket* socket)
    : DeviceLink(deviceId, parent)
    , mBluetoothSocket(new DeviceLineReader(socket))
{
    connect(mBluetoothSocket, SIGNAL(readyRead()),
            this, SLOT(dataReceived()));

    //We take ownership of the socket.
    //When the link provider distroys us,
    //the socket (and the reader) will be
    //destroyed as well
    connect(mBluetoothSocket, SIGNAL(disconnected()),
            this, SLOT(deleteLater()));
    mBluetoothSocket->setParent(this);
}

/*void BluetoothDeviceLink::sendPayload(const QSharedPointer<QIODevice>& mInput)
{
    while ( mInput->bytesAvailable() > 0 )
    {
        qint64 bytes = qMin(mInput->bytesAvailable(), (qint64)4096);
        int w = mBluetoothSocket->write(mInput->read(bytes));
        if (w<0) {
            qWarning() << "error when writing data to upload" << bytes << mInput->bytesAvailable();
            break;
        }
        else
        {
            while ( mBluetoothSocket->flush() );
        }
    }

    mInput->close();
}*/

bool BluetoothDeviceLink::sendPackageEncrypted(QCA::PublicKey& key, NetworkPackage& np)
{
    np.encrypt(key);
    return sendPackage(np);
}

bool BluetoothDeviceLink::sendPackage(NetworkPackage& np)
{
    int written = mBluetoothSocket->write(np.serialize());
    if (np.hasPayload()) {
        qWarning() << "Bluetooth backend does not support payloads.";
    }
    return (written != -1);
}

void BluetoothDeviceLink::dataReceived()
{

    if (mBluetoothSocket->bytesAvailable() == 0) return;

    const QByteArray package = mBluetoothSocket->readLine();

    //kDebug(debugArea()) << "BluetoothDeviceLink dataReceived" << package;

    NetworkPackage unserialized(QString::null);
    NetworkPackage::unserialize(package, &unserialized);
    if (unserialized.isEncrypted()) {
        //mPrivateKey should always be set when device link is added to device, no null-checking done here
        NetworkPackage decrypted(QString::null);
        unserialized.decrypt(mPrivateKey, &decrypted);

        if (decrypted.hasPayloadTransferInfo()) {
            qWarning() << "Bluetooth backend does not support payloads.";
        }

        Q_EMIT receivedPackage(decrypted);

    } else {
        if (unserialized.hasPayloadTransferInfo()) {
            qWarning() << "Ignoring unencrypted payload";
        }

        Q_EMIT receivedPackage(unserialized);

    }

    if (mBluetoothSocket->bytesAvailable() > 0) {
        QMetaObject::invokeMethod(this, "dataReceived", Qt::QueuedConnection);
    }

}
