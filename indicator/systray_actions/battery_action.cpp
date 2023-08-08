/*
 * SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2020 Piyush Aggarwal <piyushaggarwal002@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "battery_action.h"

BatteryAction::BatteryAction(DeviceDbusInterface *device)
    : QAction(nullptr)
    , m_batteryIface(device->id())
{
    setCharge(m_batteryIface.charge());
    setCharging(m_batteryIface.isCharging());

    connect(&m_batteryIface, &BatteryDbusInterface::refreshed, this, [this](bool isCharging, int charge) {
        setCharge(charge);
        setCharging(isCharging);
    });

    BatteryAction::update();
}

void BatteryAction::update()
{
    if (m_charge < 0)
        setText(i18n("No Battery"));
    else if (m_charging)
        setText(i18n("Battery: %1% (Charging)", m_charge));
    else
        setText(i18n("Battery: %1%", m_charge));

    // set icon name
    QString iconName = QStringLiteral("battery");
    if (m_charge < 0) {
        iconName += QStringLiteral("-missing");
    } else {
        int val = int(m_charge / 10) * 10;
        QString numberPaddedString = QStringLiteral("%1").arg(val, 3, 10, QLatin1Char('0'));
        iconName += QStringLiteral("-") + numberPaddedString;
    }

    if (m_charging) {
        iconName += QStringLiteral("-charging");
    }

    setIcon(QIcon::fromTheme(iconName));
}

void BatteryAction::setCharge(int charge)
{
    m_charge = charge;
    update();
}

void BatteryAction::setCharging(bool charging)
{
    m_charging = charging;
    update();
}

#include "moc_battery_action.cpp"
