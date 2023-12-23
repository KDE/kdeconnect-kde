/**
 * SPDX-FileCopyrightText: 2019 Aleix Pol Gonzalez <aleixpol@kde.org>
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
        pluginName: "sms"
    }

    readonly property variant plugin: available ? SmsDbusInterfaceFactory.create(device.id()) : null
}

