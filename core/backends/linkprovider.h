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

#include <QObject>

#include "core/networkpackage.h"
#include "pairinghandler.h"

class DeviceLink;

class KDECONNECTCORE_EXPORT LinkProvider
    : public QObject
{
    Q_OBJECT

public:

    const static int PRIORITY_LOW = 0;      //eg: 3g internet
    const static int PRIORITY_MEDIUM = 50;  //eg: internet
    const static int PRIORITY_HIGH = 100;   //eg: lan

    LinkProvider();
    ~LinkProvider() override = default;

    virtual QString name() = 0;
    virtual int priority() = 0;

public Q_SLOTS:
    virtual void onStart() = 0;
    virtual void onStop() = 0;
    virtual void onNetworkChange() = 0;

Q_SIGNALS:
    //NOTE: The provider will destroy the DeviceLink when it's no longer accessible,
    //      and every user should listen to the destroyed signal to remove its references.
    //      That's the reason because there is no "onConnectionLost".
    void onConnectionReceived(const NetworkPackage& identityPackage, DeviceLink*) const;

};

#endif
