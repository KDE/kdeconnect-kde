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
    property QtObject device
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

        Item {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            Layout.preferredHeight: 240
            Layout.preferredWidth: 240
            width: 240; height: 240
            Image {
                id: albumArt
                anchors.centerIn: parent
                width: 220; height: 220
                fillMode: Image.PreserveAspectFit
                source: root.pluginInterface.localAlbumArtUrl
                visible: !!root.pluginInterface.localAlbumArtUrl && root.pluginInterface.localAlbumArtUrl.length > 0
                smooth: true
            }
        }

        ComboBox {
            Layout.fillWidth: true
            model: root.pluginInterface.playerList
            onCurrentTextChanged: root.pluginInterface.player = currentText
            focus: true
            KeyNavigation.down: mediaBackButton
            KeyNavigation.priority: KeyNavigation.BeforeItem
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
            // player controls are by convention always LtR
            LayoutMirroring.enabled: false
            Button {
                id: mediaBackButton
                Layout.fillWidth: true
                icon.name: "media-skip-backward"
                onClicked: root.pluginInterface.sendAction("Previous")
                KeyNavigation.right: mediaPlayButton
                KeyNavigation.down: positionIndicator
                // can't use KeyNavigation as it still flips on reading direction even with LayoutMirroring disabled
                Keys.onRightPressed: mediaPlayButton.forceActiveFocus(Qt.TabFocusReason)
            }
            Button {
                id: mediaPlayButton
                Layout.fillWidth: true
                icon.name: root.pluginInterface.isPlaying ? "media-playback-pause" : "media-playback-start"
                onClicked: root.pluginInterface.sendAction("PlayPause");
                // can't use KeyNavigation as it still flips on reading direction even with LayoutMirroring disabled
                Keys.onLeftPressed: mediaBackButton.forceActiveFocus(Qt.TabFocusReason)
                Keys.onRightPressed: mediaForwardButton.forceActiveFocus(Qt.TabFocusReason)
                KeyNavigation.down: positionIndicator
            }
            Button {
                id: mediaForwardButton
                Layout.fillWidth: true
                icon.name: "media-skip-forward"
                onClicked: root.pluginInterface.sendAction("Next")
                KeyNavigation.down: positionIndicator
                // can't use KeyNavigation as it still flips on reading direction even with LayoutMirroring disabled
                Keys.onLeftPressed: mediaPlayButton.forceActiveFocus(Qt.TabFocusReason)
            }
        }
        RowLayout {
            Layout.fillWidth: true
            // media progress indicators are by convention always LtR
            LayoutMirroring.enabled: false
            LayoutMirroring.childrenInherit: true
            Label {
                text: msToTime(positionIndicator.item.value)
            }

            MprisSlider {
                id: positionIndicator
                plugin: root.pluginInterface
                Layout.fillWidth: true
                KeyNavigation.up: mediaBackButton
                KeyNavigation.down: muteButton
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
                KeyNavigation.right: volumeSlider
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
                KeyNavigation.up: positionIndicator
            }
        }
        Item { Layout.fillHeight: true }
    }

    Component.onCompleted: volumeUnmuted = root.pluginInterface.volume
}
