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

#include "../devicelink.h"

class ConnectionMultiplexer;
class MultiplexChannel;
class BluetoothLinkProvider;

class KDECONNECTCORE_EXPORT BluetoothDeviceLink : public DeviceLink
{
    Q_OBJECT

public:
    BluetoothDeviceLink(const DeviceInfo &deviceInfo,
                        BluetoothLinkProvider *parent,
                        ConnectionMultiplexer *connection,
                        QSharedPointer<MultiplexChannel> socket);

    bool sendPacket(NetworkPacket &np) override;

    DeviceInfo deviceInfo() const override
    {
        return mDeviceInfo;
    }

    QString address() const override;

private Q_SLOTS:
    void dataReceived();

private:
    ConnectionMultiplexer *mConnection;
    QSharedPointer<MultiplexChannel> mChannel;
    DeviceInfo mDeviceInfo;

    void sendMessage(const QString mMessage);
};

#endif
