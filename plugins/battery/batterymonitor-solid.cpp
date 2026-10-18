/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "batterymonitor.h"

#include <Solid/Battery>
#include <Solid/Device>
#include <Solid/Predicate>

#include "plugin_battery_debug.h"

const auto batteryDevice = Solid::DeviceInterface::Type::Battery;
const auto primary = Solid::Battery::BatteryType::PrimaryBattery;

class BatteryMonitor::Private
{
};

BatteryMonitor::BatteryMonitor(QObject *parent)
    : QObject(parent)
{
    QList<Solid::Device> batteries = Solid::Device::listFromQuery(Solid::Predicate(batteryDevice, QStringLiteral("type"), primary));

    if (batteries.isEmpty()) {
        qCWarning(KDECONNECT_PLUGIN_BATTERY) << "No Primary Battery detected on this system. This may be a bug.";
        QList<Solid::Device> allBatteries = Solid::Device::listFromType(batteryDevice);
        qCWarning(KDECONNECT_PLUGIN_BATTERY) << "Total quantity of batteries found: " << allBatteries.size();
        return;
    }

    // Ok, there's at least one. Let's assume it will remain attached (for most laptops
    // and desktops, this is a safe assumption).
    const Solid::Battery *chosen = batteries.first().as<Solid::Battery>();

    connect(chosen, &Solid::Battery::chargeStateChanged, this, &BatteryMonitor::changed);
    connect(chosen, &Solid::Battery::chargePercentChanged, this, &BatteryMonitor::changed);
}

BatteryMonitor::~BatteryMonitor() = default;

std::optional<BatteryStatus> BatteryMonitor::status() const
{
    // Note: the status returned can reflect MULTIPLE batteries.
    // We average the total charge against the total number of batteries, which in practice
    // seems to work out ok.
    bool isAnyBatteryCharging = false;
    int batteryQuantity = 0;
    int cumulativeCharge = 0;

    QList<Solid::Device> batteries = Solid::Device::listFromQuery(Solid::Predicate(batteryDevice, QStringLiteral("type"), primary));

    for (auto device : batteries) {
        const Solid::Battery *battery = device.as<Solid::Battery>();

        // Don't look at batteries that can be easily detached
        if (battery->isPowerSupply()) {
            batteryQuantity++;
            cumulativeCharge += battery->chargePercent();
            if (battery->chargeState() == Solid::Battery::ChargeState::Charging) {
                isAnyBatteryCharging = true;
            }
        }
    }

    if (batteryQuantity == 0) {
        return std::nullopt;
    }

    const Solid::Battery *chosen = batteries.first().as<Solid::Battery>();

    return BatteryStatus{
        .charge = cumulativeCharge / batteryQuantity,
        .isCharging = isAnyBatteryCharging,
        .isDischarging = chosen->chargeState() == Solid::Battery::ChargeState::Discharging,
        .batteryQuantity = batteryQuantity,
    };
}

#include "moc_batterymonitor.cpp"
