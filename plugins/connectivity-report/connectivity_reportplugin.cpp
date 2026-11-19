/**
 * SPDX-FileCopyrightText: 2021 David Shlemayev <david.shlemayev@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "connectivity_reportplugin.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>
#include <tuple>

#include <core/daemon.h>

#include "plugin_connectivity_report_debug.h"

K_PLUGIN_CLASS_WITH_JSON(ConnectivityReportPlugin, "kdeconnect_connectivity_report.json")

QString ConnectivityReportPlugin::cellularNetworkType() const
{
    return m_cellularNetworkType;
}

int ConnectivityReportPlugin::cellularNetworkStrength() const
{
    return m_cellularNetworkStrength;
}

static QString iconNameFor(const QString &networkType, int strength)
{
    const QString signalStrengthIconName = (strength < 0) ? QStringLiteral("network-mobile-off")
        : (strength == 0)                                 ? QStringLiteral("network-mobile-0")
        : (strength == 1)                                 ? QStringLiteral("network-mobile-20")
        : (strength == 2)                                 ? QStringLiteral("network-mobile-60")
        : (strength == 3)                                 ? QStringLiteral("network-mobile-80")
        : (strength == 4)                                 ? QStringLiteral("network-mobile-100")
                                                          : QStringLiteral("network-mobile-available"); // Should not happen

    const QString networkTypeSuffix = (networkType == QLatin1String("5G")) ? QStringLiteral("-5g")
        : (networkType == QLatin1String("LTE"))                            ? QStringLiteral("-lte")
        : (networkType == QLatin1String("HSPA"))                           ? QStringLiteral("-hspa")
        : (networkType == QLatin1String("UMTS"))                           ? QStringLiteral("-umts")
        : (networkType == QLatin1String("EDGE"))                           ? QStringLiteral("-edge")
        : (networkType == QLatin1String("GPRS"))                           ? QStringLiteral("-gprs")
                                                                           : QString();

    return signalStrengthIconName + networkTypeSuffix;
}

QString ConnectivityReportPlugin::iconName() const
{
    return iconNameFor(m_cellularNetworkType, m_cellularNetworkStrength);
}

QString ConnectivityReportPlugin::subscriptionsJson() const
{
    return m_subscriptionsJson;
}

void ConnectivityReportPlugin::receivePacket(const NetworkPacket &np)
{
    const auto subscriptions = np.get<QVariantMap>(QStringLiteral("signalStrengths"), QVariantMap());
    if (subscriptions.isEmpty()) {
        return;
    }

    struct Sub {
        QString name;
        QString networkType;
        int strength;
        int slot; // -1 if unknown
        int id;
    };
    QList<Sub> subs;
    for (auto it = subscriptions.cbegin(); it != subscriptions.cend(); ++it) {
        const QVariantMap info = it.value().toMap();
        subs.append({info.value(QStringLiteral("name")).toString(),
                     info.value(QStringLiteral("networkType")).toString(),
                     info.value(QStringLiteral("signalStrength")).toInt(),
                     info.value(QStringLiteral("slot"), -1).toInt(),
                     it.key().toInt()});
    }
    // Order by slot when the peer provides it, otherwise by subscription ID
    std::sort(subs.begin(), subs.end(), [](const Sub &a, const Sub &b) {
        return std::tie(a.slot, a.id) < std::tie(b.slot, b.id);
    });

    // The single-value properties report the SIM with the best signal
    const Sub *best = &subs.first();
    QJsonArray array;
    for (const Sub &sub : std::as_const(subs)) {
        if (sub.strength > best->strength) {
            best = &sub;
        }
        array.append(QJsonObject{{QStringLiteral("name"), sub.name},
                                 {QStringLiteral("networkType"), sub.networkType},
                                 {QStringLiteral("signalStrength"), sub.strength},
                                 {QStringLiteral("iconName"), iconNameFor(sub.networkType, sub.strength)}});
    }
    const QString json = QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));

    const bool changed = m_subscriptionsJson != json;
    m_cellularNetworkType = best->networkType;
    m_cellularNetworkStrength = best->strength;
    m_subscriptionsJson = json;

    if (changed) {
        Q_EMIT refreshed(m_cellularNetworkType, m_cellularNetworkStrength);
    }
}

QString ConnectivityReportPlugin::dbusPath() const
{
    return QLatin1String("/modules/kdeconnect/devices/%1/connectivity_report").arg(device()->id());
}

#include "connectivity_reportplugin.moc"
#include "moc_connectivity_reportplugin.cpp"
