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

#include "daemon.h"
#include "networkpackage.h"
#include "notificationpackagereceiver.h"
#include "pausemusicpackagereceiver.h"
#include "bonjourdevicelocator.h"
#include "fakedevicelocator.h"

#include <QtNetwork/QUdpSocket>
#include <QFile>

#include <KDE/KIcon>

#include <iostream>

K_PLUGIN_FACTORY(AndroidShineFactory,
                 registerPlugin<Daemon>();)
K_EXPORT_PLUGIN(AndroidShineFactory("androidshine", "androidshine"))


Daemon::Daemon(QObject *parent, const QList<QVariant>&)
    : KDEDModule(parent)
{

    qDebug() << "GO GO GO!";


    //TODO: Do not hardcode the load of the package receivers
    packageReceivers.push_back(new NotificationPackageReceiver());
    packageReceivers.push_back(new PauseMusicPackageReceiver());

    //TODO: Do not hardcode the load of the device locators
    deviceLocators.insert(new BonjourDeviceLocator());
    deviceLocators.insert(new FakeDeviceLocator());

    //TODO: Read paired devices from config
    pairedDevices.push_back(new Device("MyAndroid","MyAndroid"));


    //Try to link to all paired devices
    foreach (Device* device, pairedDevices) {

        DeviceLink* link = NULL;

        foreach (DeviceLocator* locator, deviceLocators) {

            if (locator->canLink(device)) {
                link = locator->link(device);
                if (link != NULL) break;
            }

        }

        if (link != NULL) linkedDevices.append(link);
        else qDebug() << "Could not link " + device->name();

    }

    foreach (DeviceLink* ld, linkedDevices) {
        foreach (PackageReceiver* pr, packageReceivers) {
            QObject::connect(ld,SIGNAL(receivedPackage(const NetworkPackage&)),
                             pr,SLOT(receivePackage(const NetworkPackage&)));
        }
    }

}

Daemon::~Daemon()
{
    qDebug() << "SAYONARA BABY";

}
