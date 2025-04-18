/*
 * SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page
{
    id: mousepad
    title: i18nd("kdeconnect-app", "Presentation Remote")
    property QtObject pluginInterface
    property QtObject device

    actions: [
        Kirigami.Action {
            icon.name: "view-fullscreen"
            text: i18nd("kdeconnect-app", "Enable Full-Screen")
            onTriggered: {
                mousepad.pluginInterface.sendKeyPress("", 25 /*XK_F5*/);
            }
        }
    ]

    ColumnLayout
    {
        anchors.fill: parent

        RowLayout {
            Layout.fillWidth: true
            Button {
                focus: true
                Layout.fillWidth: true
                icon.name: "media-skip-backward"
                Accessible.name: "Previous Slide"
                onClicked: mousepad.pluginInterface.sendKeyPress("p");
                KeyNavigation.right: forwardButton
            }
            Button {
                id: forwardButton
                Layout.fillWidth: true
                icon.name: "media-skip-forward"
                Accessible.name: "Next Slide"
                onClicked: mousepad.pluginInterface.sendKeyPress("n");
            }
        }
    }
}
