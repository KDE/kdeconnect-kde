/*
 * SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.2
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.0 as Kirigami

Kirigami.Page
{
    id: mousepad
    title: i18nd("kdeconnect-app", "Presentation Remote")
    property QtObject pluginInterface

    actions.main: Kirigami.Action {
        icon.name: "view-fullscreen"
        text: i18nd("kdeconnect-app", "Enable Full-Screen")
        onTriggered: {
            mousepad.pluginInterface.sendKeyPress("", 25 /*XK_F5*/);
        }
    }

    ColumnLayout
    {
        anchors.fill: parent

        RowLayout {
            Layout.fillWidth: true
            Button {
                Layout.fillWidth: true
                icon.name: "media-skip-backward"
                onClicked: mousepad.pluginInterface.sendKeyPress("p");
            }
            Button {
                Layout.fillWidth: true
                icon.name: "media-skip-forward"
                onClicked: mousepad.pluginInterface.sendKeyPress("n");
            }
        }
    }
}
