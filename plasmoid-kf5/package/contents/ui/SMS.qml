/**
 * SPDX-FileCopyrightText: 2019 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.kdeconnect 1.0

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

