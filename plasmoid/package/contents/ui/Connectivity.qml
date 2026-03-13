/**
 * SPDX-FileCopyrightText: 2021 David Shlemayev <david.shlemayev@gmail.com>
 * SPDX-FileCopyrightText: 2024 ivan tkachenko <me@ratijas.tk>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

pragma ComponentBehavior: Bound

import QtQuick

import org.kde.kdeconnect as KDEConnect

QtObject {
    id: root

    required property KDEConnect.DeviceDbusInterface device

    readonly property alias available: checker.available

    readonly property KDEConnect.PluginChecker pluginChecker: KDEConnect.PluginChecker {
        id: checker
        pluginName: "connectivity_report"
        device: root.device
    }

    /**
     * Reports a string indicating the network type. Possible values:
     * 5G
     * LTE
     * HSPA
     * UMTS
     * CDMA2000
     * EDGE
     * GPRS
     * GSM
     * CDMA
     * iDEN
     *
     * The parsing from Android values into these strings is handled in the
     * [ConnectivityReportPlugin.networkTypeToString method](https://invent.kde.org/network/kdeconnect-android/-/blob/master/src/org/kde/kdeconnect/Plugins/ConnectivityReportPlugin/ConnectivityReportPlugin.java#L82)
     */
    readonly property string networkType: connectivity?.cellularNetworkType ?? i18n("Unknown")

    /**
     * Reports a value between 0 and 4 (inclusive) which represents the strength of the cellular connection
     */
    readonly property int signalStrength: connectivity?.cellularNetworkStrength ?? -1
    readonly property string iconName: connectivity?.iconName ?? "network-mobile-off"

    readonly property string displayString: {
        if (connectivity !== null) {
            return `${networkType} ${signalStrength}/4`;
        } else {
            return i18n("No signal");
        }
    }

    property KDEConnect.ConnectivityReportDbusInterface connectivity

    onAvailableChanged: {
        if (available) {
            connectivity = KDEConnect.DeviceConnectivityReportDbusInterfaceFactory.create(device.id());
        } else {
            connectivity = null;
        }
    }
}
