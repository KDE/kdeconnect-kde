/**
 * SPDX-FileCopyrightText: 2020 Aniket Kumar <anikketkumar786@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.12
import org.kde.kirigami 2.13 as Kirigami
import QtMultimedia 5.12

Item {
    id: root
    property int partID
    property string mimeType
    property string uniqueIdentifier
    property string sourcePath: ""

    readonly property int elementWidth: 100
    readonly property int elementHeight: 100

    width: thumbnailElement.visible ? thumbnailElement.width : elementWidth
    height: thumbnailElement.visible ? thumbnailElement.height : elementHeight

    Component {
        id: attachmentViewer

        AttachmentViewer {
            filePath: root.sourcePath
            mimeType: root.mimeType
            title: uniqueIdentifier
        }
    }

    Image {
        id: thumbnailElement
        visible: mimeType.match("image") || mimeType.match("video")
        source: visible ? "image://thumbnailsProvider/" + root.uniqueIdentifier : ""

        property bool rounded: true
        property bool adapt: true
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter

        layer.enabled: rounded
        layer.effect: OpacityMask {
            maskSource: Item {
                width: thumbnailElement.width
                height: thumbnailElement.height
                Rectangle {
                    anchors.centerIn: parent
                    width: thumbnailElement.adapt ? thumbnailElement.width : Math.min(thumbnailElement.width, thumbnailElement.height)
                    height: thumbnailElement.adapt ? thumbnailElement.height : width
                    radius: messageBox.radius
                }
            }
        }

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
            anchors.horizontalCenter: thumbnailElement.horizontalCenter
            anchors.verticalCenter: thumbnailElement.verticalCenter
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

        Audio {
            id: audioPlayer
            source: root.sourcePath

            onStopped: {
                audioPlayButton.icon.name = "media-playback-start"
            }

            onPlaying: {
                audioPlayButton.icon.name = "media-playback-stop"
            }
        }

        ColumnLayout {
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: Kirigami.Units.largeSpacing

            Button {
                id : audioPlayButton
                icon.name: "media-playback-start"
                Layout.alignment: Qt.AlignCenter

                onClicked: {
                    if (root.sourcePath != "") {
                        if (icon.name == "media-playback-start") {
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
        onFilePathReceived: {
            if (root.uniqueIdentifier == fileName && root.sourcePath == "") {
                root.sourcePath = "file:" + filePath

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
