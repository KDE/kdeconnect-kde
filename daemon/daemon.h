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
#include "packagereceivers/packagereceiver.h"
#include "devicelinks/devicelink.h"
#include "announcers/announcer.h"

class QUdpSocket;

class Daemon
    : public KDEDModule
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect")

public:
    Daemon(QObject *parent, const QList<QVariant>&);
    ~Daemon();

private Q_SLOTS:
    void deviceConnection(DeviceLink* dl);

public Q_SLOTS:

    //After calling this, signal deviceDiscovered will be triggered for each device
    Q_SCRIPTABLE void startDiscovery(int timeOut);

    Q_SCRIPTABLE QString listVisibleDevices();

    Q_SCRIPTABLE bool pairDevice(QString id);

/*
    Q_SCRIPTABLE QString listPairedDevices(QString id);

    Q_SCRIPTABLE bool linkAllPairedDevices();
*/

    Q_SCRIPTABLE QString listLinkedDevices();

Q_SIGNALS:

    Q_SCRIPTABLE void deviceDiscovered(QString id, QString name);
    //Q_SCRIPTABLE void deviceLost(QString id);

private:


    void linkTo(DeviceLink* dl);

    KSharedConfigPtr config;

    //(Non paired?) visible devices
    QMap<QString, DeviceLink*> visibleDevices;

    //All paired devices (should be stored and read from disk)
    QVector<Device*> pairedDevices;

    //Currently connected devices
    QVector<DeviceLink*> linkedDevices;





    //Different ways to find devices and connect to them, ordered by priority
    QSet<Announcer*> announcers;

    //Everybody who wants to be notified about a new package
    QVector<PackageReceiver*> packageReceivers;

};

#endif // UDP_WIRELESS_H
