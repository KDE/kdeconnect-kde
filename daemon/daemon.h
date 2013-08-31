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
#include <QSet>
#include <QMap>
#include <QtCrypto>

#include <KDEDModule>
#include <KPluginFactory>

#include "device.h"
#include "networkpackage.h"
#include "devicelinks/devicelink.h"
#include "linkproviders/linkprovider.h"

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

    Q_SCRIPTABLE void forceOnNetworkChange();

    //Returns a list of ids. The respective devices can be manipulated using the dbus path: "/modules/kdeconnect/Devices/"+id
    Q_SCRIPTABLE QStringList devices(); //All known devices
    Q_SCRIPTABLE QStringList visibleDevices(); //Only visible devices

Q_SIGNALS:
    Q_SCRIPTABLE void deviceAdded(const QString& id);
    Q_SCRIPTABLE void deviceRemoved(const QString& id); //Note that paired devices will never be removed
    Q_SCRIPTABLE void deviceVisibilityChanged(const QString& id, bool isVisible);

private Q_SLOTS:
    void onNewDeviceLink(const NetworkPackage& identityPackage, DeviceLink* dl);
    void onDeviceReachableStatusChanged();

private:

    //Different ways to find devices and connect to them
    QSet<LinkProvider*> mLinkProviders;

    //Every known device
    QMap<QString, Device*> mDevices;

    // The Initializer object sets things up, and also does cleanup when it goes out of scope
    QCA::Initializer init;

};

#endif // UDP_WIRELESS_H
