/**
 * SPDX-FileCopyrightText: 2020 Aniket Kumar <anikketkumar786@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import org.kde.kirigami as Kirigami
import QtMultimedia

Item {
    id: root
    property int partID
    property string mimeType
    property string uniqueIdentifier
    property string sourcePath: ""

    readonly property int elementWidth: 100
    readonly property int elementHeight: 100

    width: thumbnailElement.width
    height: thumbnailElement.height

    Component {
        id: attachmentViewer

        AttachmentViewer {
            filePath: root.sourcePath
            mimeType: root.mimeType
            title: uniqueIdentifier
        }
    }

    Kirigami.ShadowedImage {
        id: thumbnailElement
        visible: mimeType.match("image") || mimeType.match("video")
        source: visible ? "image://thumbnailsProvider/" + root.uniqueIdentifier : ""

        radius: 6

        width: visible ? sourceSize.width : elementWidth
        height: visible ? sourceSize.height : elementHeight

        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (root.sourcePath == "") {
                    conversationModel.requestAttachmentPath(root.partID, root.uniqueIdentifier)
                } else {
                    openMedia();
                }
            }
        }

        Button {
            icon.name: "media-playback-start"
            visible: root.mimeType.match("video")
            anchors.centerIn: parent
            onClicked: {
                if (root.sourcePath == "") {
                    conversationModel.requestAttachmentPath(root.partID, root.uniqueIdentifier)
                } else {
                    openMedia();
                }
            }
        }
    }

    Rectangle {
        id: audioElement
        visible: root.mimeType.match("audio")
        anchors.fill: parent
        radius: messageBox.radius
        color: "lightgrey"

        MediaPlayer {
            id: audioPlayer
            source: root.sourcePath

            onPlaybackStateChanged: {
                if (playbackState === MediaPlayer.PlayingState) {
                    audioPlayButton.icon.name = "media-playback-stop"
                } else {
                    audioPlayButton.icon.name = "media-playback-start"
                }
            }
        }

        ColumnLayout {
            anchors.centerIn: parent
            spacing: Kirigami.Units.largeSpacing

            Button {
                id : audioPlayButton
                icon.name: "media-playback-start"
                Layout.alignment: Qt.AlignCenter

                onClicked: {
                    if (root.sourcePath != "") {
                        if (icon.name === "media-playback-start") {
                            audioPlayer.play()
                        } else {
                            audioPlayer.stop()
                        }
                    } else {
                        conversationModel.requestAttachmentPath(root.partID, root.uniqueIdentifier)
                    }
                }
            }

            Label {
                text: i18nd("kdeconnect-sms", "Audio clip")
            }
        }
    }

    Connections {
        target: conversationModel
        function onFilePathReceived(filePath, fileName) {
            if (root.uniqueIdentifier === fileName && root.sourcePath == "") {
                root.sourcePath = "file://" + filePath

                if (root.mimeType.match("audio")) {
                    audioPlayer.source = root.sourcePath
                    audioPlayer.play()
                } else if (root.mimeType.match("image") || root.mimeType.match("video")) {
                    openMedia();
                }
            }
        }
    }

    function openMedia() {
        applicationWindow().pageStack.layers.push(attachmentViewer)
    }
}
