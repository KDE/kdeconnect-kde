/**
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef BLUETOOTHDEVICELINK_H
#define BLUETOOTHDEVICELINK_H

#include <QObject>
#include <QString>
#include <QBluetoothSocket>

#include "../devicelink.h"
#include "../devicelinereader.h"
#include "bluetoothpairinghandler.h"

class KDECONNECTCORE_EXPORT BluetoothDeviceLink
    : public DeviceLink
{
    Q_OBJECT

public:
    BluetoothDeviceLink(const QString& deviceId, LinkProvider* parent, QBluetoothSocket* socket);

    virtual QString name() override;
    bool sendPacket(NetworkPacket& np) override;

    virtual void userRequestsPair() override;
    virtual void userRequestsUnpair() override;

    virtual bool linkShouldBeKeptAlive() override;

private Q_SLOTS:
    void dataReceived();

private:
    DeviceLineReader* mSocketReader;
    QBluetoothSocket* mBluetoothSocket;
    BluetoothPairingHandler* mPairingHandler;

    void sendMessage(const QString mMessage);

};

#endif
