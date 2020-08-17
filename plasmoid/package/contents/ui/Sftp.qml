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
        pluginName: "sftp"
    }

    property variant sftp: null

    function browse() {
        if (sftp)
            sftp.startBrowsing();
    }

    onAvailableChanged: {
        if (available) {
            sftp = SftpDbusInterfaceFactory.create(device.id())
        } else {
            sftp = null
        }
    }
}
