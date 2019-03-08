/*
 * Copyright 2016 Saikrishna Arcot <saiarcot895@gmail.com>
 * Copyright 2018 Matthijs TIjink <matthijstijink@gmail.com>
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

#include "bluetoothuploadjob.h"
#include "connectionmultiplexer.h"
#include "multiplexchannel.h"

#include <QBluetoothSocket>
#include "core_debug.h"
#include <QCoreApplication>

BluetoothUploadJob::BluetoothUploadJob(const QSharedPointer<QIODevice>& data, ConnectionMultiplexer *connection, QObject* parent)
    : QObject(parent)
    , mData(data)
    , mTransferUuid(connection->newChannel())
{
    mSocket = QSharedPointer<MultiplexChannel>{connection->getChannel(mTransferUuid).release()};
}

QVariantMap BluetoothUploadJob::transferInfo() const
{
    QVariantMap ret;
    ret[QStringLiteral("uuid")] = mTransferUuid.toString().mid(1, 36);
    return ret;
}

void BluetoothUploadJob::start()
{
    if (!mData->open(QIODevice::ReadOnly)) {
        qCWarning(KDECONNECT_CORE) << "error when opening the input to upload";
        return; //TODO: Handle error, clean up...
    }
    connect(mSocket.data(), &MultiplexChannel::bytesWritten, this, &BluetoothUploadJob::writeSome);
    connect(mSocket.data(), &MultiplexChannel::aboutToClose, this, &BluetoothUploadJob::closeConnection);
    writeSome();
}

void BluetoothUploadJob::writeSome() {
    bool errorOccurred = false;
    while (mSocket->bytesToWrite() == 0 && mData->bytesAvailable() && mSocket->isWritable()) {
        qint64 bytes = qMin<qint64>(mData->bytesAvailable(), 4096);
        int bytesWritten = mSocket->write(mData->read(bytes));

        if (bytesWritten < 0) {
            qCWarning(KDECONNECT_CORE()) << "error when writing data to bluetooth upload" << bytes << mData->bytesAvailable();
            errorOccurred = true;
            break;
        }
    }

    if (mData->atEnd() || errorOccurred) {
        mData->close();
        mSocket->close();
    }
}

void BluetoothUploadJob::closeConnection() {
    mData->close();
    deleteLater();
}
