/**
 * SPDX-FileCopyrightText: 2014 Samoilenko Yuri <kinnalru@gmail.com>
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
        pluginName: "sftp"
        device: root.device
    }

    property KDEConnect.SftpDbusInterface sftp

    function browse(): void {
        if (sftp) {
            sftp.startBrowsing();
        }
    }

    onAvailableChanged: {
        if (available) {
            sftp = KDEConnect.SftpDbusInterfaceFactory.create(device.id());
        } else {
            sftp = null;
        }
    }
}
