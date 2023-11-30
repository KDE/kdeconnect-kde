/**
 * SPDX-FileCopyrightText: 2021 Yaman Qalieh <ybq987@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import org.kde.plasma.core as PlasmaCore
import org.kde.kdeconnect

QtObject {

    id: root

    property alias device: checker.device
    readonly property alias available: checker.available

    readonly property PluginChecker pluginChecker: PluginChecker {
        id: checker
        pluginName: "clipboard"
    }

    property variant clipboard: null

    function sendClipboard() {
        if (clipboard) {
            clipboard.sendClipboard();
        }
    }

    onAvailableChanged: {
        if (available) {
            clipboard = ClipboardDbusInterfaceFactory.create(device.id())
        } else {
            clipboard = null
        }
    }
}
