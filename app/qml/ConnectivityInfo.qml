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

    readonly property string displayString: {
        if (signalStrength < 0 || connectivity === null) {
            return i18nd("kdeconnect-app", "Mobile Network: No signal");
        } else if (subscriptions.length > 1) {
            return i18nd("kdeconnect-app", "Mobile Network: %1", subscriptions.map((sim, i) => simLabel(sim, i)).join(" · "));
        } else {
            return i18nd("kdeconnect-app", "Mobile Network: %1", networkType);
        }
    }

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
