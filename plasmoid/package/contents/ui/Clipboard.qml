/**
 * SPDX-FileCopyrightText: 2021 Yaman Qalieh <ybq987@gmail.com>
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
        pluginName: "clipboard"
        device: root.device
    }

    property KDEConnect.ClipboardDbusInterface clipboard

    function sendClipboard(): void {
        if (clipboard) {
            clipboard.sendClipboard();
        }
    }

    onAvailableChanged: {
        if (available) {
            clipboard = KDEConnect.ClipboardDbusInterfaceFactory.create(device.id());
        } else {
            clipboard = null;
        }
    }
}
