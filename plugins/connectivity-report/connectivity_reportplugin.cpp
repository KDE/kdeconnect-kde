/**
 * SPDX-FileCopyrightText: 2021 David Shlemayev <david.shlemayev@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "connectivity_reportplugin.h"

#include <KLocalizedString>
#include <KPluginFactory>

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

QString ConnectivityReportPlugin::iconName() const
{
    const QString signalStrengthIconName = (m_cellularNetworkStrength < 0) ? QStringLiteral("network-mobile-off")
        : (m_cellularNetworkStrength == 0)                                 ? QStringLiteral("network-mobile-0")
        : (m_cellularNetworkStrength == 1)                                 ? QStringLiteral("network-mobile-20")
        : (m_cellularNetworkStrength == 2)                                 ? QStringLiteral("network-mobile-60")
        : (m_cellularNetworkStrength == 3)                                 ? QStringLiteral("network-mobile-80")
        : (m_cellularNetworkStrength == 4)                                 ? QStringLiteral("network-mobile-100")
                                                                           : QStringLiteral("network-mobile-available"); // Should not happen

    const QString networkTypeSuffix = (m_cellularNetworkType == QLatin1String("5G")) ? QStringLiteral("-5g")
        : (m_cellularNetworkType == QLatin1String("LTE"))                            ? QStringLiteral("-lte")
        : (m_cellularNetworkType == QLatin1String("HSPA"))                           ? QStringLiteral("-hspa")
        : (m_cellularNetworkType == QLatin1String("UMTS"))                           ? QStringLiteral("-umts")
        : (m_cellularNetworkType == QLatin1String("EDGE"))                           ? QStringLiteral("-edge")
        : (m_cellularNetworkType == QLatin1String("GPRS"))                           ? QStringLiteral("-gprs")
                                                                                     : QString();

    return signalStrengthIconName + networkTypeSuffix;
}

void ConnectivityReportPlugin::receivePacket(const NetworkPacket &np)
{
    auto subscriptions = np.get<QVariantMap>(QStringLiteral("signalStrengths"), QVariantMap());
    if (!subscriptions.isEmpty()) {
        auto networkInfo = subscriptions.first().toMap();

        const auto oldCellularNetworkType = m_cellularNetworkType;
        const auto oldNetworkStrength = m_cellularNetworkStrength;

        m_cellularNetworkType = networkInfo.value(QStringLiteral("networkType")).toString();
        m_cellularNetworkStrength = networkInfo.value(QStringLiteral("signalStrength")).toInt();

        if (oldCellularNetworkType != m_cellularNetworkType || oldNetworkStrength != m_cellularNetworkStrength) {
            Q_EMIT refreshed(m_cellularNetworkType, m_cellularNetworkStrength);
        }
    }
}

QString ConnectivityReportPlugin::dbusPath() const
{
    return QLatin1String("/modules/kdeconnect/devices/%1/connectivity_report").arg(device()->id());
}

#include "connectivity_reportplugin.moc"
#include "moc_connectivity_reportplugin.cpp"
