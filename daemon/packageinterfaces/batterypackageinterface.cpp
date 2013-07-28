/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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

#include "batterypackageinterface.h"

#include <QDebug>
#include <kicon.h>


BatteryPackageInterface::BatteryPackageInterface(QObject* parent)
    : PackageInterface(parent)
{
    //TODO: Get initial state of all devices

    QDBusConnection::sessionBus().registerObject("/modules/kdeconnect/plugins/battery", this, QDBusConnection::ExportScriptableContents);
    //The solid backend watches for this service to become available, because it is not possible to watch for a path
    QDBusConnection::sessionBus().registerService("org.kde.kdeconnect.battery");

    qDebug() << "BatteryPackageInterface registered in dbus";

}

bool BatteryPackageInterface::receivePackage(const Device& device, const NetworkPackage& np)
{
    if (np.type() != PACKAGE_TYPE_BATTERY) return false;

    QString id = device.id();

    if (!mDevices.contains(id)) {

        //TODO: Avoid ugly const_cast
        DeviceBatteryInformation* deviceInfo = new DeviceBatteryInformation(const_cast<Device*>(&device));

        mDevices[id] = deviceInfo;

        emit batteryDeviceAdded(device.id());
        connect(&device, SIGNAL(reachableStatusChanged()), this, SLOT(deviceReachableStatusChange()));

        qDebug() << "Added battery info to device" << id;

    }

    bool isCharging = np.get<bool>("isCharging");
    mDevices[id]->setCharging(isCharging);

    int currentCharge = np.get<int>("currentCharge");
    mDevices[id]->setCharge(currentCharge);

    return true;

}

QStringList BatteryPackageInterface::getBatteryReportingDevices()
{
    return mDevices.keys();
}

void BatteryPackageInterface::deviceReachableStatusChange()
{
    Device* device = static_cast<Device*>(sender());
    if (!device->reachable()) {
        mDevices.remove(device->id());
        emit batteryDeviceLost(device->id());
        disconnect(device, SIGNAL(reachableStatusChanged()), this, SLOT(deviceReachableStatusChange()));
    }
}

