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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BLUETOOTHLINKPROVIDER_H
#define BLUETOOTHLINKPROVIDER_H

#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QtBluetooth/QBluetoothAddress>
#include <QtBluetooth/QBluetoothLocalDevice>
#include <QtBluetooth/QBluetoothUuid>
#include <QtBluetooth/QBluetoothServer>
#include <QtBluetooth/QBluetoothDeviceDiscoveryAgent>
#include <QtBluetooth/QBluetoothServiceDiscoveryAgent>
#include <QtBluetooth/QBluetoothSocket>

#include "../linkprovider.h"

class BluetoothDeviceLink;

class KDECONNECTCORE_EXPORT BluetoothLinkProvider
    : public LinkProvider
{
    Q_OBJECT

public:
    BluetoothLinkProvider();
    virtual ~BluetoothLinkProvider();

    QString name() override { return "BluetoothLinkProvider"; }
    int priority() override { return PRIORITY_MEDIUM; }

public Q_SLOTS:
    virtual void onNetworkChange() override;
    virtual void onStart() override;
    virtual void onStop() override;
    void connectError();
    void serviceDiscoveryFinished();

private Q_SLOTS:
    void deviceLinkDestroyed(QObject* destroyedDeviceLink);
    void socketDisconnected();

    void serverNewConnection();
    void serverDataReceived();
    void clientConnected();
    void clientIdentityReceived();

private:
    void addLink(BluetoothDeviceLink* deviceLink, const QString& deviceId);
    QList<QBluetoothAddress> getPairedDevices();

    QBluetoothUuid mServiceUuid;
    QPointer<QBluetoothServer> mBluetoothServer;
    QBluetoothServiceInfo mKdeconnectService;
    QBluetoothServiceDiscoveryAgent* mServiceDiscoveryAgent;
    QTimer* connectTimer;

    QMap<QString, DeviceLink*> mLinks;

    QMap<QBluetoothAddress, QBluetoothSocket*> mSockets;

};

#endif
