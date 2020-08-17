/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef BATTERYDBUSINTERFACE_H
#define BATTERYDBUSINTERFACE_H

#include <QDBusAbstractAdaptor>

class Device;

class BatteryDbusInterface
    : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.battery")

public:
    explicit BatteryDbusInterface(const Device* device);
    ~BatteryDbusInterface() override;
    
    Q_SCRIPTABLE int charge() const { return m_charge; }
    Q_SCRIPTABLE bool isCharging() const { return m_isCharging; }

    void updateValues(bool isCharging, int currentCharge);

Q_SIGNALS:
    Q_SCRIPTABLE void stateChanged(bool charging);  
    Q_SCRIPTABLE void chargeChanged(int charge);

private:
    int m_charge;
    bool m_isCharging;

    // Map to save current BatteryDbusInterface for each device.
    static QMap<QString, BatteryDbusInterface *> s_dbusInterfaces;
};

#endif
