/*
 * SPDX-FileCopyrightText: 2016 Saikrishna Arcot <saiarcot895@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef BLUETOOTHDOWNLOADJOB_H
#define BLUETOOTHDOWNLOADJOB_H

#include <QIODevice>
#include <QThread>
#include <QVariantMap>
#include <QSharedPointer>
#include <QBluetoothAddress>
#include <QBluetoothUuid>
#include <QBluetoothSocket>

class ConnectionMultiplexer;
class MultiplexChannel;

class BluetoothDownloadJob
    : public QObject
{
    Q_OBJECT
public:
    explicit BluetoothDownloadJob(ConnectionMultiplexer *connection, const QVariantMap& transferInfo, QObject* parent = 0);

    QSharedPointer<QIODevice> payload() const;
    void start();
private:
    QSharedPointer<MultiplexChannel> mSocket;
};

#endif // BLUETOOTHDOWNLOADJOB_H
