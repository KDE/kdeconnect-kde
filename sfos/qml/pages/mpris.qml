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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0
import QtQuick.Layouts 1.0
import org.kde.kdeconnect 1.0

Page
{
    id: root
    property QtObject pluginInterface

    Label {
        id: noPlayersText
        text: "No players available"
        anchors.centerIn: parent
        visible: pluginInterface.playerList.length == 0
    }
    ColumnLayout
    {
        anchors.fill: parent
        anchors.margins: Theme.paddingMedium
        PageHeader { title: "Multimedia Controls" }
        visible: !noPlayersText.visible

        ComboBox {
            label: "Player"
            Layout.fillWidth: true
            onCurrentIndexChanged: root.pluginInterface.player = value

            menu: ContextMenu {
                Repeater {
                    model: root.pluginInterface.playerList
                    MenuItem { text: modelData }
                }
            }
        }

        Label {
            id: nowPlaying
            Layout.fillWidth: true
            text: root.pluginInterface.nowPlaying
            visible: root.pluginInterface.title.length == 0
            wrapMode: Text.Wrap
        }
        Label {
            Layout.fillWidth: true
            text: root.pluginInterface.title
            visible: !nowPlaying.visible
            wrapMode: Text.Wrap
        }
        Label {
            Layout.fillWidth: true
            text: root.pluginInterface.artist
            visible: !nowPlaying.visible && !artistAlbum.visible && root.pluginInterface.artist.length > 0
            wrapMode: Text.Wrap
        }
        Label {
            Layout.fillWidth: true
            text: root.pluginInterface.album
            visible: !nowPlaying.visible && !artistAlbum.visible && root.pluginInterface.album.length > 0
            wrapMode: Text.Wrap
        }
        Label {
            id: artistAlbum
            Layout.fillWidth: true
            text: "%1 - %2", root.pluginInterface.artist, root.pluginInterface.album
            visible: !nowPlaying.visible && root.pluginInterface.album.length > 0 && root.pluginInterface.artist.length > 0
            wrapMode: Text.Wrap
        }

        RowLayout {
            Layout.fillWidth: true
            height: childrenRect.height
            IconButton {
                id: btnPrev
                Layout.fillWidth: true
                icon.source: "image://theme/icon-m-previous"
                onClicked: root.pluginInterface.sendAction("Previous")
            }
            IconButton {
                id: btnPlay
                Layout.fillWidth: true
                icon.source: root.pluginInterface.isPlaying ? "image://theme/icon-m-pause" : "image://theme/icon-m-play"
                onClicked: root.pluginInterface.sendAction("PlayPause");
            }
            IconButton {
                id: btnNext
                Layout.fillWidth: true
                icon.source: "image://theme/icon-m-next"
                onClicked: root.pluginInterface.sendAction("Next")
            }
        }

        Slider {
            id: sldVolume
            label: "Volume"
            maximumValue: 100
            Layout.fillWidth: true
            //value: root.pluginInterface.volume
            onValueChanged: {
                root.pluginInterface.volume = value;
            }
        }

        Item { height: parent.height }
    }


    Connections {
        target: root.pluginInterface
        onPropertiesChanged: {
            sldVolume.value = root.pluginInterface.volume;
        }
    }

    Component.onCompleted: {
        pluginInterface.requestPlayerList();
    }
}
