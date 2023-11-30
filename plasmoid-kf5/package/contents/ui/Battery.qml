/**
 * SPDX-FileCopyrightText: 2014 Samoilenko Yuri <kinnalru@gmail.com>
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

    readonly property PluginChecker pluginChecker: PluginChecker {
        id: checker
        pluginName: "battery"
    }

    property bool charging: battery ? battery.isCharging : false
    property int charge: battery ? battery.charge : -1
    property string displayString: (available && charge > -1) ? ((charging) ? (i18n("%1% charging", charge)) : (i18n("%1%", charge))) : i18n("No info")
    property variant battery: null

    /**
     * Suggests an icon name to use for the current battery level
     */
    readonly property string iconName: {
        charge < 0 ?
          "battery-missing-symbolic" :
        charge < 10 ?
          charging ?
            "battery-empty-charging-symbolic" :
            "battery-empty-symbolic" :
        charge < 25 ?
          charging ?
            "battery-caution-charging-symbolic" :
            "battery-caution-symbolic" :
        charge < 50 ?
            charging ?
              "battery-low-charging-symbolic" :
              "battery-low-symbolic" :
        charge < 75 ?
          charging ?
            "battery-good-charging-symbolic" :
            "battery-good-symbolic" :
        charging ?
          "battery-full-charging-symbolic":
          "battery-full-symbolic"
    }

    onAvailableChanged: {
        if (available) {
            battery = DeviceBatteryDbusInterfaceFactory.create(device.id())
        } else {
            battery = null
        }
    }
}
