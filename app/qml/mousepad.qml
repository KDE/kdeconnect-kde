/*
 * SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.2
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.7 as Kirigami
import org.kde.kdeconnect 1.0

Kirigami.Page
{
    id: mousepad
    title: i18nd("kdeconnect-app", "Remote Control")
    property QtObject pluginInterface
    property QtObject device

    // Otherwise swiping on the MouseArea might trigger changing the page
    Kirigami.ColumnView.preventStealing: true

    ColumnLayout
    {
        anchors.fill: parent

        MouseArea {
            id: area
            Layout.fillWidth: true
            Layout.fillHeight: true
            property var lastPos: Qt.point(-1, -1)

            onClicked: mousepad.pluginInterface.sendCommand({"singleclick": true});

            onPositionChanged: {
                if (lastPos.x > -1) {
    //                 console.log("move", mouse.x, mouse.y, lastPos)
                    var delta = Qt.point(mouse.x-lastPos.x, mouse.y-lastPos.y);

                    pluginInterface.moveCursor(delta);
                }
                lastPos = Qt.point(mouse.x, mouse.y);
            }
            onReleased: {
                lastPos = Qt.point(-1, -1)
            }
        }

        RemoteKeyboard {
            device: mousepad.device
            Layout.fillWidth: true
            visible: remoteState
        }

        RowLayout {
            Layout.fillWidth: true

            Button {
                Layout.fillWidth: true
                icon.name: "input-mouse-click-left"
                onClicked: mousepad.pluginInterface.sendCommand({"singleclick": true});
            }
            Button {
                Layout.fillWidth: true
                icon.name: "input-mouse-click-middle"
                onClicked: mousepad.pluginInterface.sendCommand({"middleclick": true});
            }
            Button {
                Layout.fillWidth: true
                icon.name: "input-mouse-click-right"
                onClicked: mousepad.pluginInterface.sendCommand({"rightclick": true});
            }
        }
    }
}
