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

#include <KDE/KDEDModule>
#include <KDE/KNotification>

#include "networkpackage.h"
#include <KDE/KPluginFactory>
#include <QFile>
#include <qtextstream.h>

#include <QSet>

#include "device.h"
#include "packagereceiver.h"
#include "devicelink.h"
#include "devicelocator.h"

class QUdpSocket;

class Daemon
    : public KDEDModule
{
    Q_OBJECT

public:
    Daemon(QObject *parent, const QList<QVariant>&);
    ~Daemon();

//DBUS interface

public Q_SLOTS:

    Q_SCRIPTABLE QString listVisibleDevices();

    Q_SCRIPTABLE bool linkDevice(QString id);

/*
    Q_SCRIPTABLE bool pairDevice(long id);

    Q_SCRIPTABLE QString listPairedDevices(long id);

    Q_SCRIPTABLE bool linkAllPairedDevices();

*/

    Q_SCRIPTABLE QString listLinkedDevices(long id);

private:

    //Get a DeviceLink through the best DeviceLocator available
    DeviceLink* linkTo(QString d);

    //All known devices (read from/stored to settings)
    QVector<Device*> pairedDevices;

    //Currently connected devices
    QVector<DeviceLink*> linkedDevices;

    //Different ways to find new devices and connect to them, ordered by priority
    QSet<DeviceLocator*> deviceLocators;

    //Everybody who wants to be notified about a new package
    QVector<PackageReceiver*> packageReceivers;

};

#endif // UDP_WIRELESS_H
