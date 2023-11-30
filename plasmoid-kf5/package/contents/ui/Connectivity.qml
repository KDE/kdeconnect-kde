/**
 * SPDX-FileCopyrightText: 2021 David Shlemayev <david.shlemayev@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.kdeconnect 1.0

QtObject {

    id: root

    property alias device: checker.device
    readonly property alias available: checker.available
    readonly property bool ready: connectivity

    readonly property PluginChecker pluginChecker: PluginChecker {
        id: checker
        pluginName: "connectivity_report"
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
    readonly property string networkType: connectivity ? connectivity.cellularNetworkType : i18n("Unknown")

    /**
     * Reports a value between 0 and 4 (inclusive) which represents the strength of the cellular connection
     */
    readonly property int signalStrength: connectivity ? connectivity.cellularNetworkStrength : -1
    property string displayString: {
        if (ready) {
            return `${networkType} ${signalStrength}/4`;
        } else {
            return i18n("No signal");
        }
    }
    property variant connectivity: null

    /**
     * Suggests an icon name to use for the current signal level
     *
     * Returns names which correspond to Plasma Framework's network.svg:
     * https://invent.kde.org/frameworks/plasma-framework/-/blob/master/src/desktoptheme/breeze/icons/network.svg
     */
    readonly property string iconName: {
        // Firstly, get the name prefix which represents the signal strength
        var signalStrengthIconName =
            (signalStrength < 0 || !ready) ?
                // As long as the signal strength is nonsense or the plugin reports as non-ready,
                // show us as disconnected
                "network-mobile-off" :
            (signalStrength == 0) ?
                "network-mobile-0" :
            (signalStrength == 1) ?
                "network-mobile-20" :
            (signalStrength == 2) ?
                "network-mobile-60" :
            (signalStrength == 3) ?
                "network-mobile-80" :
            (signalStrength == 4) ?
                "network-mobile-100" :
            // Since all possible values are enumerated above, this default case should never be hit.
            // However, I need it in order for my ternary syntax to be valid!
            "network-mobile-available"

        // If we understand the network type, append to the icon name to show the type
        var networkTypeSuffix =
            (networkType === "5G") ?
                // No icon for this case!
                "" :
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
                "" // We didn't recognize the network type. Don't append anything.
        return signalStrengthIconName + networkTypeSuffix
    }

    onAvailableChanged: {
        if (available) {
            connectivity = DeviceConnectivityReportDbusInterfaceFactory.create(device.id())
        } else {
            connectivity = null
        }
    }
}
