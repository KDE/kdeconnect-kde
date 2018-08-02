/*
 * Copyright 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0
import org.kde.kdeconnect 1.0

Page
{
    id: root
    property QtObject pluginInterface

    Column
    {
        anchors.fill: parent
        PageHeader { title: "Multimedia Controls" }

        Component.onCompleted: {
            pluginInterface.requestPlayerList();
        }

        Item { height: parent.height }
        ComboBox {
            label: "Player"
            width: parent.width
            onCurrentIndexChanged: root.pluginInterface.player = value

            menu: ContextMenu {
                Repeater {
                    model: root.pluginInterface.playerList
                    MenuItem { text: modelData }
                }
            }
        }
        Label {
            width: parent.width
            text: root.pluginInterface.nowPlaying
        }
        Row {
            width: parent.width
            IconButton {
                icon.source: "image://theme/icon-m-previous"
                onClicked: root.pluginInterface.sendAction("Previous")
            }
            IconButton {
                icon.source: root.pluginInterface.isPlaying ? "icon-m-image://theme/pause" : "image://theme/icon-m-play"
                onClicked: root.pluginInterface.sendAction("PlayPause");
            }
            IconButton {
                icon.source: "image://theme/icon-m-next"
                onClicked: root.pluginInterface.sendAction("Next")
            }
        }
        Row {
            width: parent.width
            Label { text: ("Volume:") }
            Slider {
                value: root.pluginInterface.volume
                maximumValue: 100
                width: parent.width
            }
        }
        Item { height: parent.height }
    }
}
