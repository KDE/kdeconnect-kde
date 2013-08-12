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

#ifndef DEVICEBATTERYINFORMATION_H
#define DEVICEBATTERYINFORMATION_H

#include <QObject>
#include <QDBusAbstractAdaptor>

class DeviceBatteryInformation
    : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.battery")
    Q_PROPERTY( int charge READ charge NOTIFY chargingChange )
    Q_PROPERTY( bool isCharging READ isCharging NOTIFY chargingChange )

public:
    DeviceBatteryInformation(QObject* parent = 0);

    int charge() { return mCharge; }
    void setCharge(int charge) { mCharge = charge; emit chargingChange(); }
    bool isCharging() { return mIsCharging; }
    void setCharging(bool isCharging) { mIsCharging = isCharging; emit chargingChange(); }

private:
    bool mIsCharging;
    int mCharge;

Q_SIGNALS:
    Q_SCRIPTABLE void chargingChange();

};

#endif //DEVICEBATTERYINFORMATION_H