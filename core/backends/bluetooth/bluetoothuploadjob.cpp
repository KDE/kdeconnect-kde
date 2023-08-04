/*
 * SPDX-FileCopyrightText: 2016 Saikrishna Arcot <saiarcot895@gmail.com>
 * SPDX-FileCopyrightText: 2018 Matthijs TIjink <matthijstijink@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "bluetoothuploadjob.h"
#include "connectionmultiplexer.h"
#include "multiplexchannel.h"

#include "core_debug.h"
#include <QBluetoothSocket>
#include <QCoreApplication>

BluetoothUploadJob::BluetoothUploadJob(const QSharedPointer<QIODevice> &data, ConnectionMultiplexer *connection, QObject *parent)
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
        return; // TODO: Handle error, clean up...
    }
    connect(mSocket.data(), &MultiplexChannel::bytesWritten, this, &BluetoothUploadJob::writeSome);
    connect(mSocket.data(), &MultiplexChannel::aboutToClose, this, &BluetoothUploadJob::closeConnection);
    writeSome();
}

void BluetoothUploadJob::writeSome()
{
    bool errorOccurred = false;
    while (mSocket->bytesToWrite() == 0 && mData->bytesAvailable() && mSocket->isWritable()) {
        qint64 bytes = qMin<qint64>(mData->bytesAvailable(), 4096);
        int bytesWritten = mSocket->write(mData->read(bytes));

        if (bytesWritten < 0) {
            qCWarning(KDECONNECT_CORE) << "error when writing data to bluetooth upload" << bytes << mData->bytesAvailable();
            errorOccurred = true;
            break;
        }
    }

    if (mData->atEnd() || errorOccurred) {
        mData->close();
        mSocket->close();
    }
}

void BluetoothUploadJob::closeConnection()
{
    mData->close();
    deleteLater();
}

#include "moc_bluetoothuploadjob.cpp"
