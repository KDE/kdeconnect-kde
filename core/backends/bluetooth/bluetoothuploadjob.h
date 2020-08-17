/*
 * SPDX-FileCopyrightText: 2016 Saikrishna Arcot <saiarcot895@gmail.com>
 * SPDX-FileCopyrightText: 2018 Matthijs TIjink <matthijstijink@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef BLUETOOTHUPLOADJOB_H
#define BLUETOOTHUPLOADJOB_H

#include <QIODevice>
#include <QThread>
#include <QVariantMap>
#include <QSharedPointer>
#include <QBluetoothAddress>
#include <QBluetoothUuid>
#include <QBluetoothServer>

class ConnectionMultiplexer;
class MultiplexChannel;

class BluetoothUploadJob
    : public QObject
{
    Q_OBJECT
public:
    explicit BluetoothUploadJob(const QSharedPointer<QIODevice>& data, ConnectionMultiplexer *connection, QObject* parent = 0);

    QVariantMap transferInfo() const;
    void start();

private:
    QSharedPointer<QIODevice> mData;
    QBluetoothUuid mTransferUuid;
    QSharedPointer<MultiplexChannel> mSocket;

    void closeConnection();

private Q_SLOTS:
    void writeSome();
};

#endif // BLUETOOTHUPLOADJOB_H
