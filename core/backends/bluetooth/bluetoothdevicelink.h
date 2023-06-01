/**
 * SPDX-FileCopyrightText: 2016 Saikrishna Arcot <saiarcot895@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef BLUETOOTHDEVICELINK_H
#define BLUETOOTHDEVICELINK_H

#include <QBluetoothSocket>
#include <QObject>
#include <QSslCertificate>
#include <QString>

#include "../devicelinereader.h"
#include "../devicelink.h"

class ConnectionMultiplexer;
class MultiplexChannel;

class KDECONNECTCORE_EXPORT BluetoothDeviceLink : public DeviceLink
{
    Q_OBJECT

public:
    BluetoothDeviceLink(const QString &deviceId, LinkProvider *parent, ConnectionMultiplexer *connection, QSharedPointer<MultiplexChannel> socket);

    virtual QString name() override;
    bool sendPacket(NetworkPacket &np) override;

    QSslCertificate certificate() const override;

private Q_SLOTS:
    void dataReceived();

private:
    DeviceLineReader *mSocketReader;
    ConnectionMultiplexer *mConnection;
    QSharedPointer<MultiplexChannel> mChannel;

    void sendMessage(const QString mMessage);
};

#endif
