/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "batteryplugin.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <Solid/Battery>
#include <Solid/Device>
#include <Solid/Predicate>

#include <core/daemon.h>

#include "plugin_battery_debug.h"

K_PLUGIN_CLASS_WITH_JSON(BatteryPlugin, "kdeconnect_battery.json")

const auto batteryDevice = Solid::DeviceInterface::Type::Battery;
const auto primary = Solid::Battery::BatteryType::PrimaryBattery;

BatteryPlugin::BatteryPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
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

    connect(chosen, &Solid::Battery::chargeStateChanged, this, &BatteryPlugin::slotChargeChanged);
    connect(chosen, &Solid::Battery::chargePercentChanged, this, &BatteryPlugin::slotChargeChanged);
}

int BatteryPlugin::charge() const
{
    return m_charge;
}

bool BatteryPlugin::isCharging() const
{
    return m_isCharging;
}

void BatteryPlugin::connected()
{
    // Explicitly send the current charge
    slotChargeChanged();
}

void BatteryPlugin::slotChargeChanged()
{
    // Note: the NetworkPacket sent at the end of this method can reflect MULTIPLE batteries.
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
        qCWarning(KDECONNECT_PLUGIN_BATTERY) << "Primary Battery seems to have been removed. Suspending packets until it is reconnected.";
        return;
    }

    // Load a new Battery object to represent the first device in the list
    Solid::Battery *chosen = batteries.first().as<Solid::Battery>();

    // Prepare an outgoing network packet
    NetworkPacket status(PACKET_TYPE_BATTERY, {{}});
    status.set(QStringLiteral("isCharging"), isAnyBatteryCharging);
    const int charge = cumulativeCharge / batteryQuantity;
    status.set(QStringLiteral("currentCharge"), charge);
    // FIXME: In future, we should consider sending an array of battery objects
    status.set(QStringLiteral("batteryQuantity"), batteryQuantity);
    // We consider the primary battery to be low if it's below 15%
    if (charge <= 15 && chosen->chargeState() == Solid::Battery::ChargeState::Discharging) {
        status.set(QStringLiteral("thresholdEvent"), (int)ThresholdBatteryLow);
    } else {
        status.set(QStringLiteral("thresholdEvent"), (int)ThresholdNone);
    }
    sendPacket(status);
}

void BatteryPlugin::receivePacket(const NetworkPacket &np)
{
    m_isCharging = np.get<bool>(QStringLiteral("isCharging"), false);
    m_charge = np.get<int>(QStringLiteral("currentCharge"), -1);
    const int thresholdEvent = np.get<int>(QStringLiteral("thresholdEvent"), (int)ThresholdNone);

    Q_EMIT refreshed(m_isCharging, m_charge);

    if (thresholdEvent == ThresholdBatteryLow && !m_isCharging) {
        Daemon::instance()->sendSimpleNotification(QStringLiteral("batteryLow"),
                                                   i18nc("device name: low battery", "%1: Low Battery", device()->name()),
                                                   i18n("Battery at %1%", m_charge),
                                                   QStringLiteral("battery-040"));
    }
}

QString BatteryPlugin::dbusPath() const
{
    return QLatin1String("/modules/kdeconnect/devices/%1/battery").arg(device()->id());
}

#include "batteryplugin.moc"
#include "moc_batteryplugin.cpp"
