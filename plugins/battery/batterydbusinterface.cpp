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

#include "batterydbusinterface.h"
#include "batteryplugin.h"

#include <QDebug>
#include <core/device.h>

QMap<QString, BatteryDbusInterface *> BatteryDbusInterface::s_dbusInterfaces;

BatteryDbusInterface::BatteryDbusInterface(const Device* device)
    : QDBusAbstractAdaptor(const_cast<Device*>(device))
	, m_charge(-1)
	, m_isCharging(false)
{
    // FIXME: Workaround to prevent memory leak.
    // This makes the old BatteryDdbusInterface be deleted only after the new one is
    // fully operational. That seems to prevent the crash mentioned in BatteryPlugin's
    // destructor.
    QMap<QString, BatteryDbusInterface *>::iterator oldInterfaceIter = s_dbusInterfaces.find(device->id());
    if (oldInterfaceIter != s_dbusInterfaces.end()) {
        qCDebug(KDECONNECT_PLUGIN_BATTERY) << "Deleting stale BatteryDbusInterface for" << device->name();
        //FIXME: This still crashes sometimes even after the workaround made in 38aa970, commented out by now
        //oldInterfaceIter.value()->deleteLater();
        s_dbusInterfaces.erase(oldInterfaceIter);
    }

    s_dbusInterfaces[device->id()] = this;
}

BatteryDbusInterface::~BatteryDbusInterface()
{
    qCDebug(KDECONNECT_PLUGIN_BATTERY) << "Destroying BatteryDbusInterface";
}

void BatteryDbusInterface::updateValues(bool isCharging, int currentCharge)
{
    m_isCharging = isCharging;
    m_charge = currentCharge;

    Q_EMIT stateChanged(m_isCharging);
    Q_EMIT chargeChanged(m_charge);
}


