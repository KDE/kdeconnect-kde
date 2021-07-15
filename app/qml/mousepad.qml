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
            property var pressedPos: Qt.point(-1, -1)
            property var releasedPos: Qt.point(-1, -1)

            acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton

            onClicked: {
                var clickType = "";
                var packet = {};
                switch (mouse.button) {
                case Qt.LeftButton:
                    if (pressedPos == releasedPos) {
                        clickType = "singleclick";
                        pressedPos = Qt.point(-1, -1);
                        releasedPos = Qt.point(-1, -1);
                    }
                    break;
                case Qt.RightButton:
                    clickType = "rightclick";
                    break;
                case Qt.MiddleButton:
                    clickType = "middleclick";
                    break;
                default:
                    console.log("This click input is not handled yet!");
                    break;
                }

                if (clickType) {
                    packet[clickType] = true;
                    mousepad.pluginInterface.sendCommand(packet);
                }
            }

            onDoubleClicked: {
                var clickType = "";
                var packet = {};
                switch (mouse.button) {
                case Qt.LeftButton:
                    clickType = "singlehold";
                    break;
                default:
                    console.log("This click input is not handled yet!");
                    break;
                }

                if (clickType) {
                    packet[clickType] = true;
                    mousepad.pluginInterface.sendCommand(packet);
                }
            }

            onPositionChanged: {
                if (lastPos.x > -1) {
    //                 console.log("move", mouse.x, mouse.y, lastPos)
                    var delta = Qt.point(mouse.x-lastPos.x, mouse.y-lastPos.y);

                    pluginInterface.moveCursor(delta);
                }
                lastPos = Qt.point(mouse.x, mouse.y);
            }

            onPressed: {
                pressedPos = Qt.point(mouse.x, mouse.y);
            }

            onWheel: {
                var packet = {};
                packet["scroll"] = true;
                packet["dy"] = wheel.angleDelta.y;
                packet["dx"] = wheel.angleDelta.x;
                mousepad.pluginInterface.sendCommand(packet);
            }

            onReleased: {
                lastPos = Qt.point(-1, -1)
                releasedPos = Qt.point(mouse.x, mouse.y);
                mousepad.pluginInterface.sendCommand({"singlerelease": true});
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
