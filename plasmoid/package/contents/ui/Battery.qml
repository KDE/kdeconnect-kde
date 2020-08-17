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

    property bool charging: false
    property int charge: -1
    property string displayString: (available && charge > -1) ? ((charging) ? (i18n("%1% charging", charge)) : (i18n("%1%", charge))) : i18n("No info")
    property variant battery: null

    property variant nested1: DBusAsyncResponse {
        id: startupCheck1
        autoDelete: false
        onSuccess: root.charging = result
    }
    
    property variant nested2: DBusAsyncResponse {
        id: startupCheck2
        autoDelete: false
        onSuccess: root.charge = result
    }
    
    onAvailableChanged: {
        if (available) {
            battery = DeviceBatteryDbusInterfaceFactory.create(device.id())
            
            battery.stateChanged.connect(function(c) {charging = c})
            battery.chargeChanged.connect(function(c) {charge = c})

            startupCheck1.setPendingCall(battery.isCharging())
            startupCheck2.setPendingCall(battery.charge())
        } else {
            battery = null
        }
    }
}
