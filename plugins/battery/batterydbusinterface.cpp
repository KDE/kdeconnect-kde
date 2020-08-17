/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "batterydbusinterface.h"
#include "batteryplugin.h"

#include <QDebug>
#include <core/device.h>
#include "plugin_battery_debug.h"

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


