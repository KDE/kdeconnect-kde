/*
 * SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

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

    function padTimeNumber(n) {
        return (n < 10) ? ("0" + n) : n;
    }

    function msToTime(totalTimeMs)
    {
        let hours = Math.floor(totalTimeMs/(1000*60*60));
        let minutes = Math.floor((totalTimeMs-(hours*1000*60*60))/(1000*60));
        let seconds = Math.floor((totalTimeMs-(minutes*1000*60)-(hours*1000*60*60))/1000);
        if (hours > 0) {
            return `${padTimeNumber(hours)}:${padTimeNumber(minutes)}:${padTimeNumber(seconds)}`;
        } else {
            return `${padTimeNumber(minutes)}:${padTimeNumber(seconds)}`;
        }
    }

    Kirigami.PlaceholderMessage {
        id: noPlayersText
        // FIXME: not accessible. screen readers won't read this.
        //        https://invent.kde.org/frameworks/kirigami/-/merge_requests/1482
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
            Layout.fillWidth: true
            text: root.pluginInterface.title
            wrapMode: Text.Wrap
        }
        Label {
            Layout.fillWidth: true
            text: root.pluginInterface.artist
            visible: !artistAlbum.visible && root.pluginInterface.artist.length > 0
            wrapMode: Text.Wrap
        }
        Label {
            Layout.fillWidth: true
            text: root.pluginInterface.album
            visible: !artistAlbum.visible && root.pluginInterface.album.length > 0
            wrapMode: Text.Wrap
        }
        Label {
            id: artistAlbum
            Layout.fillWidth: true
            text: i18nd("kdeconnect-app", "%1 - %2", root.pluginInterface.artist, root.pluginInterface.album)
            visible: root.pluginInterface.album.length > 0 && root.pluginInterface.artist.length > 0
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
                text: msToTime(positionIndicator.item.value)
            }

            MprisSlider {
                id: positionIndicator
                plugin: root.pluginInterface
                Layout.fillWidth: true
            }

            Label {
                text: msToTime(root.pluginInterface.length)
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
