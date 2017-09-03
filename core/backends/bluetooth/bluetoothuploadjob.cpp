/*
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

#include "bluetoothuploadjob.h"

#include <QBluetoothSocket>
#include "core_debug.h"
#include <QCoreApplication>

BluetoothUploadJob::BluetoothUploadJob(const QSharedPointer<QIODevice>& data, const QBluetoothAddress& remoteAddress, QObject* parent)
    : QObject(parent)
    , mData(data)
    , mRemoteAddress(remoteAddress)
    , mTransferUuid(QBluetoothUuid::createUuid())
    , mServer(new QBluetoothServer(QBluetoothServiceInfo::RfcommProtocol, this))
{
    mServer->setSecurityFlags(QBluetooth::Encryption | QBluetooth::Secure);
}

QVariantMap BluetoothUploadJob::transferInfo() const
{
    QVariantMap ret;
    ret["uuid"] = mTransferUuid.toString().mid(1, 36);
    return ret;
}

void BluetoothUploadJob::start()
{
    connect(mServer, SIGNAL(newConnection()), this, SLOT(newConnection()));
    mServiceInfo = mServer->listen(mTransferUuid, "KDE Connect Transfer Job");
    Q_ASSERT(mServiceInfo.isValid());
}

void BluetoothUploadJob::newConnection()
{
    QBluetoothSocket* socket = mServer->nextPendingConnection();
    connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));

    if (socket->peerAddress() != mRemoteAddress) {
        socket->close();
    } else {
        mServer->close();
        disconnect(mServer, SIGNAL(newConnection()), this, SLOT(newConnection()));
        mServiceInfo.unregisterService();

        if (!mData->open(QIODevice::ReadOnly)) {
            qCWarning(KDECONNECT_CORE) << "error when opening the input to upload";
            socket->close();
            deleteLater();
            return;
        }
    }

    while (mData->bytesAvailable() > 0 && socket->isWritable()) {
        qint64 bytes = qMin(mData->bytesAvailable(), (qint64)4096);
        int w = socket->write(mData->read(bytes));
        if (w < 0) {
            qCWarning(KDECONNECT_CORE()) << "error when writing data to upload" << bytes << mData->bytesAvailable();
            break;
        } else {
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 2000);
        }
    }

    mData->close();
    socket->close();
    deleteLater();
}
