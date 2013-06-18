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
#include "avahidevicelocator.h"
#include "fakedevicelocator.h"

#include <QtNetwork/QUdpSocket>
#include <QFile>

#include <KDE/KIcon>

#include <sstream>
#include <iomanip>
#include <iostream>

K_PLUGIN_FACTORY(AndroidShineFactory, registerPlugin<Daemon>();)
K_EXPORT_PLUGIN(AndroidShineFactory("androidshine", "androidshine"))

DeviceLink* Daemon::linkTo(QString id) {

    DeviceLink* link = NULL;

    Q_FOREACH (DeviceLocator* locator, deviceLocators) {
        if (locator->canLink(id)) {
            link = locator->link(id);
            if (link != NULL) break;
        }
    }

    if (link != NULL) {

        linkedDevices.append(link);

        Q_FOREACH (PackageReceiver* pr, packageReceivers) {
            QObject::connect(link,SIGNAL(receivedPackage(const NetworkPackage&)),
                             pr,SLOT(receivePackage(const NetworkPackage&)));
        }

    } else {
        qDebug() << "Could not link device id " + id;
    }

    return link;

}

Daemon::Daemon(QObject *parent, const QList<QVariant>&)
    : KDEDModule(parent)
{

    qDebug() << "GO GO GO!";

    //TODO: Do not hardcode the load of the package receivers
    packageReceivers.push_back(new NotificationPackageReceiver());
    packageReceivers.push_back(new PauseMusicPackageReceiver());

    //TODO: Do not hardcode the load of the device locators
    deviceLocators.insert(new AvahiDeviceLocator());
    deviceLocators.insert(new FakeDeviceLocator());

    //TODO: Read paired devices from config
    pairedDevices.push_back(new Device("MyAndroid","MyAndroid"));

    //At boot time, try to link to all paired devices
    Q_FOREACH (Device* device, pairedDevices) {
        linkTo(device->id());
    }

}


QString Daemon::listVisibleDevices()
{

    std::stringstream ret;

    ret << std::setw(20) << "ID";
    ret << std::setw(20) << "Name";
    ret << std::setw(20) << "Source";
    ret << std::endl;

    QList<Device*> visibleDevices;

    Q_FOREACH (DeviceLocator* dl, deviceLocators) {
        Q_FOREACH (Device* d, dl->discover()) {
            ret << std::setw(20) << d->id().toStdString();
            ret << std::setw(20) << d->name().toStdString();
            ret << std::setw(20) << dl->getName().toStdString();
            ret << std::endl;
        }
    }

    return QString::fromStdString(ret.str());

}

bool Daemon::linkDevice(QString id)
{

    return linkTo(id);

}

QString Daemon::listLinkedDevices(long int id)
{
    QString ret;

    Q_FOREACH (DeviceLink* dl, linkedDevices) {
        ret += dl->device()->name() + "(" + dl->device()->id() + ")";
    }

    return ret;
}


Daemon::~Daemon()
{
    qDebug() << "SAYONARA BABY";

}

