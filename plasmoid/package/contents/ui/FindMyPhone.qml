/**
 * SPDX-FileCopyrightText: 2014 Samoilenko Yuri <kinnalru@gmail.com>
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
        pluginName: "findmyphone"
    }

    property variant findMyPhone: null

    function ring() {
        if (findMyPhone) {
            findMyPhone.ring();
        }
    }

    onAvailableChanged: {
        if (available) {
            findMyPhone = FindMyPhoneDbusInterfaceFactory.create(device.id())
        } else {
            findMyPhone = null
        }
    }
}
