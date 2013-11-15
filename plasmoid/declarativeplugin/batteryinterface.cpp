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

#include "batteryinterface.h"

BatteryInterface::BatteryInterface(QObject* parent)
    : QObject(parent)
    , m_device(0)
    , m_battery(0)
{

}

void BatteryInterface::setDevice(const QString& deviceId)
{
    if (m_device) {
        delete m_device;
    }
    m_device = new DeviceDbusInterface(deviceId, this);
    connect(m_device, SIGNAL(pluginsChanged()), this, SLOT(devicePluginsChanged()));
    devicePluginsChanged();
}

void BatteryInterface::devicePluginsChanged()
{
    if (m_device->hasPlugin("kdeconnect_battery")) {
        m_battery = new DeviceBatteryDbusInterface(m_device->id(), this);
        connect(m_battery, SIGNAL(chargingChange()), this, SIGNAL(infoChanged()));
        Q_EMIT infoChanged();
    } else {
        delete m_battery;
        m_battery = 0;
    }

    Q_EMIT availableChanged();
}

bool BatteryInterface::available() const
{
    return m_battery && m_battery->isValid();
}

QString BatteryInterface::displayString() const
{

    if (!m_battery) {
        return i18n("No info");
    }

    if (isCharging()) {
        return i18n("Charging, %1%", chargePercent());
    } else {
        return i18n("Discharging, %1%", chargePercent());
    }

}
