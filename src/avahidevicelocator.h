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

#ifndef AVAHIDEVICELOCATOR_H
#define AVAHIDEVICELOCATOR_H

#include <QObject>
#include <KDE/DNSSD/ServiceBrowser>

#include <KDE/DNSSD/RemoteService>

#include "devicelocator.h"

class AvahiDeviceLocator
    : public DeviceLocator
{
    Q_OBJECT

public:
    AvahiDeviceLocator();
    QString getName() { return "Avahi"; }
    Priority getPriority() { return PRIORITY_HIGH; }
    bool canLink(QString id);
    DeviceLink* link(QString id);
    bool pair(Device* d);
    QList<Device*> discover() { return visibleDevices.values(); }

private slots:
    void deviceDetected(DNSSD::RemoteService::Ptr s);
    void deviceLost(DNSSD::RemoteService::Ptr s);

private:
    DNSSD::ServiceBrowser* browser;

    Device* getDeviceInfo(DNSSD::RemoteService::Ptr s);

    QMap<QString, Device*> visibleDevices;
    QMap<Device*, DNSSD::RemoteService::Ptr> deviceRoutes;

    QList<DeviceLink*> linkedDevices;

};

#endif // AVAHIDEVICELOCATOR_H
