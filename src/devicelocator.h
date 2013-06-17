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

#ifndef DEVICELOCATOR_H
#define DEVICELOCATOR_H

#include <qvector.h>
#include <QObject>

#include "devicelink.h"
#include "device.h"

class DeviceLocator
    : public QObject
{
    Q_OBJECT

public:
    DeviceLocator();
    virtual ~DeviceLocator() { }

    virtual QString getName() = 0;

    enum Priority {
        PRIORITY_LOW = 0,      //ie: 3g
        PRIORITY_MEDIUM = 50,  //ie: internet
        PRIORITY_HIGH = 100    //ie: lan
    };

    virtual Priority getPriority() = 0;
    virtual bool canLink(QString id) = 0;
    virtual DeviceLink* link(QString id) = 0;
    virtual bool pair(Device* d) = 0;
    virtual QList<Device*> discover() = 0;

};

#endif // DEVICELOCATOR_H
