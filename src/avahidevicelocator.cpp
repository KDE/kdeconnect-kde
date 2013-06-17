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

#include "avahidevicelocator.h"

#include "udpdevicelink.h"


AvahiDeviceLocator::AvahiDeviceLocator()
{
    //http://api.kde.org/4.x-api/kdelibs-apidocs/dnssd/html/index.html
    browser = new DNSSD::ServiceBrowser("_device._tcp");
    connect(browser, SIGNAL(serviceAdded(DNSSD::RemoteService::Ptr)),this,SLOT(deviceDetected(DNSSD::RemoteService::Ptr)));
    connect(browser, SIGNAL(serviceRemoved(DNSSD::RemoteService::Ptr)),this,SLOT(deviceLost(DNSSD::RemoteService::Ptr)));
    browser->startBrowse();
}

bool AvahiDeviceLocator::canLink(QString id) {
    return visibleDevices.contains(id);
}

DeviceLink* AvahiDeviceLocator::link(QString id) {

    if (!visibleDevices.contains(id)) return NULL;

    Device* d = visibleDevices[id];
    const DNSSD::RemoteService::Ptr& rs = deviceRoutes[d];

    DeviceLink* dl = new UdpDeviceLink(QHostAddress(rs->hostName()),rs->port());
    linkedDevices.append(dl); //Store the ref to be able to delete the memory later

    return dl;
}

bool AvahiDeviceLocator::pair(Device* d)
{
    //TODO: ?
    d->pair();
    return true;
}

void AvahiDeviceLocator::deviceDetected(DNSSD::RemoteService::Ptr s)
{
    qDebug() << "DETECTED";

    Device* d = getDeviceInfo(s);

    if (!visibleDevices.contains(d->id())) {
        visibleDevices[d->id()] = d;
        deviceRoutes[d] = s;
    } else {
        qDebug() << "Warning: duplicate device id " + d->id();
    }
}

void AvahiDeviceLocator::deviceLost(DNSSD::RemoteService::Ptr s)
{
    qDebug() << "LOST";

    Device* found = NULL;

    for(QMap< Device*, DNSSD::RemoteService::Ptr >::iterator it = deviceRoutes.begin(), itEnd = deviceRoutes.end(); it != itEnd; it++) {
        if (s == it.value()) {
            found = it.key();
        }
    }

    if (found != NULL) {
        visibleDevices.remove(found->id());
        deviceRoutes.remove(found);
        delete found; //created at getDeviceInfo called from deviceDetected
    } else {
        //TODO: Add a TAG to the debug messages
        qDebug() << "Warning: lost unknown device?";
    }
}

Device* AvahiDeviceLocator::getDeviceInfo(DNSSD::RemoteService::Ptr s)
{
    //TODO: I have no idea on how to get this info using only DNSSD, wihtout connecting to the device
    Device* d = new Device("1234567890", "Unnamed"); //deleted at deviceLost
    return d;
}












