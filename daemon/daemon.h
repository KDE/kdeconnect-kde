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


#ifndef DAEMON_H
#define DAEMON_H

#include <QObject>
#include <QRegExp>
#include <map>

#include <KDEDModule>
#include <KNotification>

#include "networkpackage.h"
#include <KDE/KPluginFactory>
#include <QFile>
#include <qtextstream.h>

#include <QSet>

#include <KConfig>

#include "device.h"
#include "packageinterfaces/packageinterface.h"
#include "devicelinks/devicelink.h"
#include "linkproviders/linkprovider.h"

class QUdpSocket;

class Daemon
    : public KDEDModule
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.daemon")

public:
    Daemon(QObject *parent, const QList<QVariant>&);
    ~Daemon();

public Q_SLOTS:

    //After calling this, signal deviceDiscovered will be triggered for each device
    Q_SCRIPTABLE void setDiscoveryEnabled(bool b);

    //Returns a list of ids. The respective devices can be manipulated using the dbus path: "/modules/kdeconnect/Devices/"+id
    Q_SCRIPTABLE QStringList devices();

Q_SIGNALS:
    Q_SCRIPTABLE void newDeviceAdded(const QString& id);
    Q_SCRIPTABLE void deviceStatusChanged(const QString& id);


private Q_SLOTS:
    void onNewDeviceLink(const NetworkPackage& identityPackage, DeviceLink* dl);

private:


    //Every known device
    QMap<QString, Device*> mDevices;

    //Different ways to find devices and connect to them
    QSet<LinkProvider*> mLinkProviders;

    //The classes that send and receive the packages
    QVector<PackageInterface*> mPackageInterfaces;

};

#endif // UDP_WIRELESS_H
