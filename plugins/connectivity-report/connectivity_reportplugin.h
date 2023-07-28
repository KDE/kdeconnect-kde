/**
 * SPDX-FileCopyrightText: 2021 David Shlemayev <david.shlemayev@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <core/kdeconnectplugin.h>

/**
 * Packet used to report the current connectivity state
 * <p>
 * The body should contain a key "signalStrengths" which has a dict that maps
 * a SubscriptionID (opaque value) to a dict with the connection info (See below)
 * <p>
 * For example:
 * {
 *     "signalStrengths": {
 *         "6": {
 *             "networkType": "4G",
 *             "signalStrength": 3
 *         },
 *         "17": {
 *             "networkType": "HSPA",
 *             "signalStrength": 2
 *         },
 *         ...
 *     }
 * }
 */
#define PACKET_TYPE_CONNECTIVITY_REPORT QStringLiteral("kdeconnect.connectivity_report")

class ConnectivityReportPlugin : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.connectivity_report")
    Q_PROPERTY(QString cellularNetworkType READ cellularNetworkType NOTIFY refreshed)
    Q_PROPERTY(int cellularNetworkStrength READ cellularNetworkStrength NOTIFY refreshed)

public:
    using KdeConnectPlugin::KdeConnectPlugin;

    void receivePacket(const NetworkPacket &np) override;
    QString dbusPath() const override;

    QString cellularNetworkType() const;
    int cellularNetworkStrength() const;

Q_SIGNALS:
    Q_SCRIPTABLE void refreshed(QString cellularNetworkType, int cellularNetworkStrength);

private:
    QString m_cellularNetworkType;
    int m_cellularNetworkStrength = -1;
};
