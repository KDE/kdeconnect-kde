/*
 * SPDX-FileCopyrightText: 2016 Saikrishna Arcot <saiarcot895@gmail.com>
 * SPDX-FileCopyrightText: 2018 Matthijs TIjink <matthijstijink@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef BLUETOOTHUPLOADJOB_H
#define BLUETOOTHUPLOADJOB_H

#include <QBluetoothAddress>
#include <QBluetoothServer>
#include <QBluetoothUuid>
#include <QIODevice>
#include <QSharedPointer>
#include <QThread>
#include <QVariantMap>

class ConnectionMultiplexer;
class MultiplexChannel;

class BluetoothUploadJob : public QObject
{
    Q_OBJECT
public:
    explicit BluetoothUploadJob(const QSharedPointer<QIODevice> &data, ConnectionMultiplexer *connection, QObject *parent = nullptr);

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
