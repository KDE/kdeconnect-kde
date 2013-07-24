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

#ifndef LINKPROVIDER_H
#define LINKPROVIDER_H

#include <qvector.h>
#include <QObject>

#include "devicelinks/devicelink.h"
#include "device.h"

class DeviceLink;

class LinkProvider
    : public QObject
{
    Q_OBJECT

public:

    const int PRIORITY_LOW = 0;      //eg: 3g
    const int PRIORITY_MEDIUM = 50;  //eg: internet
    const int PRIORITY_HIGH = 100;   //eg: lan

    LinkProvider();
    virtual ~LinkProvider() { }

    virtual QString name() = 0;
    virtual int priority() = 0;

    virtual void setDiscoverable(bool b) = 0;

Q_SIGNALS:
    //NOTE: The provider will to destroy the DeviceLink when it's no longer accessible,
    //      and every user should listen to the destroyed signal to remove its references.
    void onNewDeviceLink(const NetworkPackage& identityPackage, DeviceLink*);

};

#endif
