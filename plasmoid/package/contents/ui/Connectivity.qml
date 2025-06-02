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

    readonly property string displayString: {
        if (connectivity !== null) {
            return `${networkType} ${signalStrength}/4`;
        } else {
            return i18n("No signal");
        }
    }

    property KDEConnect.ConnectivityReportDbusInterface connectivity

    /**
     * Suggests an icon name to use for the current signal level
     *
     * Returns names which correspond to Plasma Framework's network.svg:
     * https://invent.kde.org/frameworks/plasma-framework/-/blob/master/src/desktoptheme/breeze/icons/network.svg
     */
    readonly property string iconName: {
        // Firstly, get the name prefix which represents the signal strength
        const signalStrengthIconName =
            (signalStrength < 0 || connectivity === null) ?
                // As long as the signal strength is nonsense or the plugin reports as non-ready,
                // show us as disconnected
                "network-mobile-off" :
            (signalStrength === 0) ?
                "network-mobile-0" :
            (signalStrength === 1) ?
                "network-mobile-20" :
            (signalStrength === 2) ?
                "network-mobile-60" :
            (signalStrength === 3) ?
                "network-mobile-80" :
            (signalStrength === 4) ?
                "network-mobile-100" :
            // Since all possible values are enumerated above, this default case should never be hit.
            // However, I need it in order for my ternary syntax to be valid!
            "network-mobile-available";

        // If we understand the network type, append to the icon name to show the type
        const networkTypeSuffix =
            (networkType === "5G") ?
                "-5g" :
            (networkType === "LTE") ?
                "-lte" :
            (networkType === "HSPA") ?
                "-hspa" :
            (networkType === "UMTS") ?
                "-umts" :
            (networkType === "CDMA2000") ?
                // GSconnect just uses the 3g icon
                // No icon for this case!
                "" :
            (networkType === "EDGE") ?
                "-edge" :
            (networkType === "GPRS") ?
                "-gprs" :
            (networkType === "GSM") ?
                // GSconnect just uses the 2g icon
                // No icon for this case!
                "" :
            (networkType === "CDMA") ?
                // GSconnect just uses the 2g icon
                // No icon for this case!
                "" :
            (networkType === "iDEN") ?
                // GSconnect just uses the 2g icon
                // No icon for this case!
                "" :
                ""; // We didn't recognize the network type. Don't append anything.

        return signalStrengthIconName + networkTypeSuffix;
    }

    onAvailableChanged: {
        if (available) {
            connectivity = KDEConnect.DeviceConnectivityReportDbusInterfaceFactory.create(device.id());
        } else {
            connectivity = null;
        }
    }
}
