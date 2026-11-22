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

    readonly property string networkType: connectivity?.cellularNetworkType ?? i18nd("kdeconnect-app", "Unknown")
    readonly property int signalStrength: connectivity?.cellularNetworkStrength ?? -1
    readonly property string iconName: connectivity?.iconName ?? "network-mobile-off"

    // One entry per SIM: {name, networkType, signalStrength, iconName}
    readonly property var subscriptions: {
        try {
            return JSON.parse(connectivity?.subscriptionsJson ?? "[]");
        } catch (e) {
            return [];
        }
    }

    function simLabel(sim, index) {
        const name = sim.name || i18nd("kdeconnect-app", "SIM %1", index + 1);
        return sim.signalStrength < 0
            ? i18nd("kdeconnect-app", "%1: No signal", name)
            : i18nd("kdeconnect-app", "%1: %2", name, sim.networkType);
    }

    readonly property var segments: {
        if (connectivity !== null && subscriptions.length > 0) {
            return subscriptions.map((sim, i) => ({ iconName: sim.iconName, text: simLabel(sim, i) }));
        }
        const text = (signalStrength < 0 || connectivity === null) ? i18nd("kdeconnect-app", "No signal") : networkType;
        return [{ iconName: iconName, text: text }];
    }

    readonly property string displayString: segments.map(segment => segment.text).join(" · ")

    readonly property KDEConnect.PluginChecker pluginChecker: KDEConnect.PluginChecker {
        id: checker
        pluginName: "connectivity_report"
        device: root.device
    }

    property KDEConnect.ConnectivityReportDbusInterface connectivity

    onAvailableChanged: {
        if (available && device) {
            connectivity = KDEConnect.DeviceConnectivityReportDbusInterfaceFactory.create(device.id());
        } else {
            connectivity = null;
        }
    }
}
