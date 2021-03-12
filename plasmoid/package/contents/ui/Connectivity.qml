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
    readonly property bool ready: connectivity && connectivity.cellularNetworkType != "Unknown" && connectivity.cellularNetworkStrength > -1

    readonly property PluginChecker pluginChecker: PluginChecker {
        id: checker
        pluginName: "connectivity_report"
    }

    property string networkType: connectivity ? connectivity.cellularNetworkType : i18n("Unknown")
    property int signalStrength: connectivity ? connectivity.cellularNetworkStrength : -1
    property string displayString: {
        if (ready) {
            return `${networkType} ${signalStrength}/4`;
        } else {
            return i18n("No signal");
        }
    }
    property variant connectivity: null

    onAvailableChanged: {
        if (available) {
            connectivity = DeviceConnectivityReportDbusInterfaceFactory.create(device.id())
        } else {
            connectivity = null
        }
    }
}
