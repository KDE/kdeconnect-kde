/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "batteryplugin.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <Solid/Device>
#include <Solid/Battery>
#include <Solid/Predicate>

#include <core/daemon.h>

#include "batterydbusinterface.h"
#include "plugin_battery_debug.h"

K_PLUGIN_CLASS_WITH_JSON(BatteryPlugin, "kdeconnect_battery.json")

BatteryPlugin::BatteryPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
    , batteryDbusInterface(new BatteryDbusInterface(device()))
{

    //TODO: Our protocol should support a dynamic number of batteries.
    //Solid::DeviceNotifier *notifier = Solid::DeviceNotifier::instance();

}

void BatteryPlugin::connected()
{
    NetworkPacket np(PACKET_TYPE_BATTERY_REQUEST, {{QStringLiteral("request"),true}});
    sendPacket(np);

    const auto batteryDevice = Solid::DeviceInterface::Type::Battery;
    const auto primary = Solid::Battery::BatteryType::PrimaryBattery;

    QList<Solid::Device> batteries = Solid::Device::listFromQuery(Solid::Predicate(batteryDevice, QStringLiteral("type"), primary));

    if (batteries.isEmpty()) {
        qCWarning(KDECONNECT_PLUGIN_BATTERY) << "No Primary Battery detected on this system. This may be a bug.";
        QList<Solid::Device> allBatteries = Solid::Device::listFromType(batteryDevice);
        qCWarning(KDECONNECT_PLUGIN_BATTERY) << "Total quantity of batteries found: " << allBatteries.size();
        return;
    }

    const Solid::Battery* chosen = batteries.first().as<Solid::Battery>();

    connect(chosen, &Solid::Battery::chargeStateChanged, this, &BatteryPlugin::chargeChanged);
    connect(chosen, &Solid::Battery::chargePercentChanged, this, &BatteryPlugin::chargeChanged);

    // Explicitly send the current charge
    chargeChanged();
}

void BatteryPlugin::chargeChanged()
{

    bool isAnyBatteryCharging = false;
    int batteryQuantity = 0;
    int cumulativeCharge = 0;

    const auto batteryDevice = Solid::DeviceInterface::Type::Battery;
    const auto primary = Solid::Battery::BatteryType::PrimaryBattery;

    QList<Solid::Device> batteries = Solid::Device::listFromQuery(Solid::Predicate(batteryDevice, QStringLiteral("type"), primary));

    for (auto device : batteries) {
        const Solid::Battery* battery = device.as<Solid::Battery>();

        // Don't look at batteries that have been detached
        if (battery->isPresent()) {
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
    Solid::Battery* chosen = batteries.first().as<Solid::Battery>();

    // Prepare an outgoing network packet
    NetworkPacket status(PACKET_TYPE_BATTERY, {{}});
    status.set(QStringLiteral("isCharging"), isAnyBatteryCharging);
    status.set(QStringLiteral("currentCharge"), cumulativeCharge / batteryQuantity);
    // FIXME: In future, we should consider sending an array of battery objects
    status.set(QStringLiteral("batteryQuantity"), batteryQuantity);
    // We consider primary battery to be low if it will only last for 5 minutes or
    // less. This doesn't necessarily work if (for example) Solid finds multiple
    // batteries.
    if (chosen->remainingTime() < 600 && chosen->chargeState() == Solid::Battery::ChargeState::Discharging) {
        status.set(QStringLiteral("thresholdEvent"), (int)ThresholdBatteryLow);
    } else {
        status.set(QStringLiteral("thresholdEvent"), (int)ThresholdNone);
    }
    sendPacket(status);
}

BatteryPlugin::~BatteryPlugin()
{
    //FIXME: Qt dbus does not allow to remove an adaptor! (it causes a crash in
    // the next dbus access to its parent). The implication of not deleting this
    // is that disabling the plugin does not remove the interface (that will
    // return outdated values) and that enabling it again instantiates a second
    // adaptor. This is also a memory leak until the entire device is destroyed.

    //batteryDbusInterface->deleteLater();
}

bool BatteryPlugin::receivePacket(const NetworkPacket& np)
{
    bool isCharging = np.get<bool>(QStringLiteral("isCharging"), false);
    int currentCharge = np.get<int>(QStringLiteral("currentCharge"), -1);
    int thresholdEvent = np.get<int>(QStringLiteral("thresholdEvent"), (int)ThresholdNone);

    if (batteryDbusInterface->charge() != currentCharge
        || batteryDbusInterface->isCharging() != isCharging
    ) {
        batteryDbusInterface->updateValues(isCharging, currentCharge);
    }

    if ( thresholdEvent == ThresholdBatteryLow && !isCharging ) {
        Daemon::instance()->sendSimpleNotification(QStringLiteral("batteryLow"), i18nc("device name: low battery", "%1: Low Battery", device()->name()), i18n("Battery at %1%", currentCharge), QStringLiteral("battery-040"));
    }

    return true;

}

#include "batteryplugin.moc"
