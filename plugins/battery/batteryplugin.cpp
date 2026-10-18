/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "batteryplugin.h"

#include <QCoreApplication>

#include <KLocalizedString>
#include <KPluginFactory>

#include <core/daemon.h>

#include "batterymonitor.h"
#include "plugin_battery_debug.h"

K_PLUGIN_CLASS_WITH_JSON(BatteryPlugin, "kdeconnect_battery.json")

BatteryPlugin::BatteryPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , m_monitor(new BatteryMonitor(this))
{
    connect(m_monitor, &BatteryMonitor::changed, this, &BatteryPlugin::slotChargeChanged);
}

int BatteryPlugin::charge() const
{
    return m_charge;
}

bool BatteryPlugin::isCharging() const
{
    return m_isCharging;
}

bool BatteryPlugin::hasBattery() const
{
    return m_charge >= 0;
}

QString BatteryPlugin::iconName() const
{
    // Suggests an icon name to use for the current battery level.
    if (m_charge < 0) {
        return QStringLiteral("battery-missing-symbolic");
    } else if (m_charge < 10) {
        return m_isCharging ? QStringLiteral("battery-empty-charging-symbolic") : QStringLiteral("battery-empty-symbolic");
    } else if (m_charge < 25) {
        return m_isCharging ? QStringLiteral("battery-caution-charging-symbolic") : QStringLiteral("battery-caution-symbolic");
    } else if (m_charge < 50) {
        return m_isCharging ? QStringLiteral("battery-low-charging-symbolic") : QStringLiteral("battery-low-symbolic");
    } else if (m_charge < 75) {
        return m_isCharging ? QStringLiteral("battery-good-charging-symbolic") : QStringLiteral("battery-good-symbolic");
    } else {
        return m_isCharging ? QStringLiteral("battery-full-charging-symbolic") : QStringLiteral("battery-full-symbolic");
    }
}

void BatteryPlugin::connected()
{
    // Explicitly send the current charge
    slotChargeChanged();
}

void BatteryPlugin::slotChargeChanged()
{
    const std::optional<BatteryStatus> battery = m_monitor->status();
    if (!battery) {
        qCWarning(KDECONNECT_PLUGIN_BATTERY) << "Primary Battery seems to have been removed. Suspending packets until it is reconnected.";
        return;
    }

    // Prepare an outgoing network packet
    NetworkPacket status(PACKET_TYPE_BATTERY, {{}});
    status.set(QStringLiteral("isCharging"), battery->isCharging);
    status.set(QStringLiteral("currentCharge"), battery->charge);
    // FIXME: In future, we should consider sending an array of battery objects
    status.set(QStringLiteral("batteryQuantity"), battery->batteryQuantity);
    // We consider the primary battery to be low if it's below 15%
    if (battery->charge <= 15 && battery->isDischarging) {
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
        static const bool s_plasma = QStringView(qEnvironmentVariable("XDG_CURRENT_DESKTOP")).split(QLatin1Char(';')).contains(QLatin1String("KDE"));
        // Disabled because Solid takes care of showing a similar notification already
        if (s_plasma && QCoreApplication::instance()->property("_kde_solid_has_kdeconnect_backend").toBool()) {
            return;
        }

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
