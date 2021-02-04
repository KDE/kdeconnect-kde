/*
 * SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.2
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.12 as Kirigami

Kirigami.Page
{
    id: root
    property QtObject pluginInterface
    property bool muted: false
    property int volumeUnmuted
    property var volume: pluginInterface.volume
    title: i18nd("kdeconnect-app", "Multimedia Controls")

    onVolumeChanged: {
        if (muted && volume != 0) {
            toggleMute()
            volumeUnmuted = volume
        } else if (!muted) {
            volumeUnmuted = volume
        }
    }

    function soundState(volume)
    {
        if (volume <= 25) {
            return "audio-volume-low"
        } else if (volume <= 75) {
            return "audio-volume-medium"
        } else {
            return "audio-volume-high"
        }
    }

    function toggleMute()
    {
        muted = !muted
        root.pluginInterface.volume = muted ? 0 : volumeUnmuted
    }

    function msToTime(currentTime, totalTime)
    {
        if (totalTime.getHours() >= 2) {
            // Skip a day ahead as Date type's minimum is 1am on the 1st of January and can't go lower
            currentTime.setDate(2)
            currentTime.setHours(currentTime.getHours() - 1)
            return currentTime.toLocaleTimeString(Qt.locale(),"hh:mm:ss")
        } else {
            return currentTime.toLocaleTimeString(Qt.locale(),"mm:ss")
        }
    }

    Kirigami.PlaceholderMessage {
        id: noPlayersText
        text: i18nd("kdeconnect-app", "No players available")
        anchors.centerIn: parent
        visible: pluginInterface.playerList.length == 0
    }

    ColumnLayout
    {
        anchors.fill: parent
        visible: !noPlayersText.visible

        Component.onCompleted: {
            pluginInterface.requestPlayerList();
        }

        Item { Layout.fillHeight: true }
        ComboBox {
            Layout.fillWidth: true
            model: root.pluginInterface.playerList
            onCurrentTextChanged: root.pluginInterface.player = currentText
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
            text: i18nd("kdeconnect-app", "%1 - %2", root.pluginInterface.artist, root.pluginInterface.album)
            visible: !nowPlaying.visible && root.pluginInterface.album.length > 0 && root.pluginInterface.artist.length > 0
            wrapMode: Text.Wrap
        }
        RowLayout {
            Layout.fillWidth: true
            Button {
                Layout.fillWidth: true
                icon.name: "media-skip-backward"
                onClicked: root.pluginInterface.sendAction("Previous")
            }
            Button {
                Layout.fillWidth: true
                icon.name: root.pluginInterface.isPlaying ? "media-playback-pause" : "media-playback-start"
                onClicked: root.pluginInterface.sendAction("PlayPause");
            }
            Button {
                Layout.fillWidth: true
                icon.name: "media-skip-forward"
                onClicked: root.pluginInterface.sendAction("Next")
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Label {
                text: msToTime(new Date(positionIndicator.item.value), new Date(root.pluginInterface.length))
            }

            MprisSlider {
                id: positionIndicator
                plugin: root.pluginInterface
                Layout.fillWidth: true
            }

            Label {
                text: msToTime(new Date(root.pluginInterface.length), new Date(root.pluginInterface.length))
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Button {
                id: muteButton
                icon.name: muted ? "audio-volume-muted" : soundState(root.pluginInterface.volume)
                onClicked: toggleMute()
            }
            Slider {
                id: volumeSlider
                value: volumeUnmuted
                to: 99
                enabled: !muted
                Layout.fillWidth: true
                onValueChanged: {
                    volumeUnmuted = value
                    root.pluginInterface.volume = value
                }
            }
        }
        Item { Layout.fillHeight: true }
    }

    Component.onCompleted: volumeUnmuted = root.pluginInterface.volume
}
