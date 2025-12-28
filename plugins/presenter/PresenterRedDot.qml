/**
 * SPDX-FileCopyrightText: 2019 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.5
import QtQuick.Window 2.5
import QtQuick.Particles 2.0

Item {
    id: root
    anchors.fill: parent

    property real xPos: 0.5
    property real yPos: 0.5

    property real devicePixelRatio: Window.window != null && Window.window.screen != null ? Window.window.screen.devicePixelRatio : 1.0

    Rectangle {
        width: parent.width/150
        height: width
        radius: width

        x: root.width * root.xPos - width/2
        y: root.height * root.yPos - height/2

        border.color: "white"
        border.width: 1
        color: "red"
        opacity: 0.8
    }
}
