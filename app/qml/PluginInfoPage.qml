/*
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: root
    property string configFile
    property string device

    actions: loader.item && loader.item.action ? [loader.item.action] : []

    onConfigFileChanged: loader.setSource(configFile, {
        device: root.device
    })

    Loader {
        anchors.fill: parent
        id: loader
        Component.onCompleted: setSource(configFile, {
            device: root.device
        })
    }
}



