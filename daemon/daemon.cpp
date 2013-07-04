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
#include "packagereceivers/pingpackagereceiver.h"
#include "packagereceivers/notificationpackagereceiver.h"
#include "packagereceivers/pausemusicpackagereceiver.h"
#include "announcers/avahiannouncer.h"
#include "announcers/fakeannouncer.h"
#include "devicelinks/echodevicelink.h"

#include <QtNetwork/QUdpSocket>
#include <QFile>
#include <QDBusConnection>

#include <KIcon>
#include <KConfigGroup>

#include <sstream>
#include <iomanip>
#include <iostream>

K_PLUGIN_FACTORY(KdeConnectFactory, registerPlugin<Daemon>();)
K_EXPORT_PLUGIN(KdeConnectFactory("kdeconnect", "kdeconnect"))

Daemon::Daemon(QObject *parent, const QList<QVariant>&)
    : KDEDModule(parent)
{

    KSharedConfigPtr config = KSharedConfig::openConfig("kdeconnectrc");

    //Debugging
    qDebug() << "GO GO GO!";
    config->group("devices").group("paired").group("fake_unreachable").writeEntry("name","Fake device");

    //TODO: Do not hardcode the load of the package receivers
    //use: https://techbase.kde.org/Development/Tutorials/Services/Plugins
    packageReceivers.push_back(new PingPackageReceiver());
    packageReceivers.push_back(new NotificationPackageReceiver());
    packageReceivers.push_back(new PauseMusicPackageReceiver());

    //TODO: Do not hardcode the load of the device locators
    //use: https://techbase.kde.org/Development/Tutorials/Services/Plugins
    announcers.insert(new AvahiAnnouncer());
    announcers.insert(new FakeAnnouncer());

    //TODO: Add package emitters

    //Read remebered paired devices
    const KConfigGroup& known = config->group("devices").group("paired");
    const QStringList& list = known.groupList();
    const QString defaultName("unnamed");
    Q_FOREACH(const QString& id, list) {
        const KConfigGroup& data = known.group(id);
        const QString& name = data.readEntry<QString>("name",defaultName);
        Device* device = new Device(id,name);
        m_devices[id] = device;
        Q_FOREACH (PackageReceiver* pr, packageReceivers) {
            connect(device,SIGNAL(receivedPackage(const Device&, const NetworkPackage&)),
                    pr,SLOT(receivePackage(const Device&, const NetworkPackage&)));
        }
    }

    //Listen to incomming connections
    Q_FOREACH (Announcer* a, announcers) {
        connect(a,SIGNAL(onNewDeviceLink(QString,QString,DeviceLink*)),
                this,SLOT(onNewDeviceLink(QString,QString,DeviceLink*)));
    }

    QDBusConnection::sessionBus().registerService("org.kde.kdeconnect");

    setDiscoveryEnabled(true);

}
void Daemon::setDiscoveryEnabled(bool b)
{
    qDebug() << "Discovery:" << b;
    //Listen to incomming connections
    Q_FOREACH (Announcer* a, announcers) {
        a->setDiscoverable(b);
    }

}

QStringList Daemon::devices()
{
    return m_devices.keys();
}


void Daemon::onNewDeviceLink(const QString& id, const QString& name, DeviceLink* dl)
{
    qDebug() << "Device discovered" << dl->deviceId();

    if (m_devices.contains(dl->deviceId())) {
        qDebug() << "It is a known device";

        Device* device = m_devices[dl->deviceId()];
        device->addLink(dl);

        KNotification* notification = new KNotification("pingReceived"); //KNotification::Persistent
        notification->setPixmap(KIcon("dialog-ok").pixmap(48, 48));
        notification->setComponentData(KComponentData("kdeconnect", "kdeconnect"));
        notification->setTitle(device->name());
        notification->setText("Succesfully connected");
        notification->sendEvent();

        emit deviceStatusChanged(id);
    } else {
        qDebug() << "It is a new device";

        Device* device = new Device(id,name,dl);
        m_devices[dl->deviceId()] = device;
        Q_FOREACH (PackageReceiver* pr, packageReceivers) {
            connect(device,SIGNAL(receivedPackage(const Device&, const NetworkPackage&)),
                    pr,SLOT(receivePackage(const Device&, const NetworkPackage&)));
        }
        emit newDeviceAdded(id);
    }

}

Daemon::~Daemon()
{
    qDebug() << "SAYONARA BABY";
}

