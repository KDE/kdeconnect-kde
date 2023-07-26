/*
 * SPDX-FileCopyrightText: 2016 Saikrishna Arcot <saiarcot895@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "bluetoothdownloadjob.h"
#include "connectionmultiplexer.h"
#include "multiplexchannel.h"

BluetoothDownloadJob::BluetoothDownloadJob(ConnectionMultiplexer *connection, const QVariantMap &transferInfo, QObject *parent)
    : QObject(parent)
{
    QBluetoothUuid id{transferInfo.value(QStringLiteral("uuid")).toString()};
    mSocket = QSharedPointer<MultiplexChannel>{connection->getChannel(id).release()};
}

QSharedPointer<QIODevice> BluetoothDownloadJob::payload() const
{
    return mSocket.staticCast<QIODevice>();
}

void BluetoothDownloadJob::start()
{
}

#include "moc_bluetoothdownloadjob.cpp"
