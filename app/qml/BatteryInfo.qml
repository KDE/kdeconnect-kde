/*
 * SPDX-FileCopyrightText: 2026 KDE Connect contributors
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
    readonly property bool charging: battery?.isCharging ?? false
    readonly property int charge: battery?.charge ?? -1
    readonly property bool hasBattery: battery?.hasBattery ?? false
    readonly property string iconName: battery?.iconName ?? "battery-missing-symbolic"

    readonly property string displayString: {
        if (charge < 0) {
            return i18nd("kdeconnect-app", "Battery: No info");
        } else if (charging) {
            return i18nd("kdeconnect-app", "Battery: %1% (Charging)", charge);
        } else {
            return i18nd("kdeconnect-app", "Battery: %1%", charge);
        }
    }

    readonly property KDEConnect.PluginChecker pluginChecker: KDEConnect.PluginChecker {
        id: checker
        pluginName: "battery"
        device: root.device
    }

    property KDEConnect.BatteryDbusInterface battery

    onAvailableChanged: {
        if (available && device) {
            battery = KDEConnect.DeviceBatteryDbusInterfaceFactory.create(device.id());
        } else {
            battery = null;
        }
    }
}
