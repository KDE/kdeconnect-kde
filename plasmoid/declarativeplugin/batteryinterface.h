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

#ifndef BATTERYINTERFACE_H
#define BATTERYINTERFACE_H

#include <QObject>
#include <QString>
#include <KLocalizedString>
#include "libkdeconnect/dbusinterfaces.h"

//Wrapper to use it from QML
class BatteryInterface
    : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString device READ device WRITE setDevice NOTIFY deviceChanged)
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(QString displayString READ displayString NOTIFY infoChanged)
    //Q_PROPERTY(bool displayString READ isCharging NOTIFY infoChanged)
    //Q_PROPERTY(int chargePercent READ chargePercent NOTIFY infoChanged)

public:
    BatteryInterface(QObject* parent = 0);
    void setDevice(const QString& deviceId);
    QString device() const { return m_device->id(); }

    bool available() const; //True if the interface is accessible

    QString displayString() const; //Human readable string with the battery status
    bool isCharging() const { return m_battery->isCharging(); }
    int chargePercent() const { return m_battery->charge(); }

Q_SIGNALS:
    void availableChanged();
    void infoChanged();
    void deviceChanged();

private:
    DeviceDbusInterface* m_device;
    mutable DeviceBatteryDbusInterface* m_battery;

public Q_SLOTS:
    void devicePluginsChanged();
};

#endif // BATTERYINTERFACE_H
