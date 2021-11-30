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

ConnectivityReportPlugin::ConnectivityReportPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
}

QString ConnectivityReportPlugin::cellularNetworkType() const
{
    return m_cellularNetworkType;
}

int ConnectivityReportPlugin::cellularNetworkStrength() const
{
    return m_cellularNetworkStrength;
}

void ConnectivityReportPlugin::connected()
{
    // We've just connected. Request connectivity_report information from the remote device...
    NetworkPacket np(PACKET_TYPE_CONNECTIVITY_REPORT_REQUEST, {{QStringLiteral("request"),true}});
    sendPacket(np);
}

bool ConnectivityReportPlugin::receivePacket(const NetworkPacket& np)
{
    if (np.type() == PACKET_TYPE_CONNECTIVITY_REPORT) {
        auto subscriptions = np.get<QVariantMap>(QStringLiteral("signalStrengths"), QVariantMap());
        auto networkInfo = subscriptions.first().toMap();

        const auto oldCellularNetworkType = m_cellularNetworkType;
        const auto oldNetworkStrength = m_cellularNetworkStrength;

        m_cellularNetworkType = networkInfo.value(QStringLiteral("networkType")).toString();
        m_cellularNetworkStrength = networkInfo.value(QStringLiteral("signalStrength")).toInt();

        if (oldCellularNetworkType != m_cellularNetworkType ||
            oldNetworkStrength != m_cellularNetworkStrength) {
          Q_EMIT refreshed(m_cellularNetworkType, m_cellularNetworkStrength);
        }
    }

    return true;
}

QString ConnectivityReportPlugin::dbusPath() const
{
    return QStringLiteral("/modules/kdeconnect/devices/") + device()->id() + QStringLiteral("/connectivity_report");
}

#include "connectivity_reportplugin.moc"
