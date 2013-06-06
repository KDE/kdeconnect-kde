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

#include "bonjourdevicelocator.h"

#include "udpdevicelink.h"

BonjourDeviceLocator::BonjourDeviceLocator()
{

/*
    //http://api.kde.org/4.x-api/kdelibs-apidocs/dnssd/html/index.html
    DNSSD::ServiceBrowser* browser = new DNSSD::ServiceBrowser("_http._tcp");
    connect(browser, SIGNAL(serviceAdded(RemoteService::Ptr)),
        this,    SLOT(addService(RemoteService::Ptr)));
    connect(browser, SIGNAL(serviceRemoved(RemoteService::Ptr)),
        this,    SLOT(delService(RemoteService::Ptr)));
    browser->startBrowse();
    In above example addService() will
*/
}

DeviceLink* BonjourDeviceLocator::link(Device* d) {
    DeviceLink* dl = new UdpDeviceLink("192.168.1.48");
    return dl;
}

bool BonjourDeviceLocator::pair(Device* d) {
    return true;
}

QVector<Device*> BonjourDeviceLocator::discover() {
    QVector<Device*> devices;
    return devices;
}
