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

Kirigami.Page {
    id: root

    property url fileUrl
    property string mimeType

    actions: [
        Kirigami.Action {
            text: i18nd("kdeconnect-sms", "Open with default")
            icon.name: "window-new"
            onTriggered: {
                Qt.openUrlExternally(root.fileUrl);
            }
        }
    ]

    contentItem: Rectangle {
        anchors.fill: parent

        Rectangle {
            id: imageViewer
            visible: root.mimeType.match("image")
            anchors.horizontalCenter: parent.horizontalCenter
            width: image.width
            height: parent.height - y
            y: root.implicitHeaderHeight
            color: parent.color

            Image {
                id: image
                source: parent.visible ? root.fileUrl : ""
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                width: sourceSize.width
                height: parent.height
                fillMode: Image.PreserveAspectFit
            }
        }

        MediaPlayer {
            id: mediaPlayer
            source: root.fileUrl

            onPositionChanged: {
                if (mediaPlayer.position > 1000 && mediaPlayer.duration - mediaPlayer.position < 1000) {
                    playAndPauseButton.icon.name = "media-playback-start"
                    mediaPlayer.pause()
                    mediaPlayer.seek(0)
                 }
            }
        }

        Item {
            width: parent.width
            height: parent.height - mediaControls.height
            anchors.topMargin: root.implicitHeaderHeight

            MediaPlayer {
                source: mediaPlayer
                videoOutput: VideoOutput {
                    anchors.fill: parent
                    fillMode: VideoOutput.PreserveAspectFit

                    // By default QML's videoOutput element rotates the vdeeo files by 90 degrees in clockwise direction
                    orientation: -90
                }
            }
        }

        Rectangle {
            id: mediaControls
            visible: root.mimeType.match("video")
            width: parent.width
            height: 50
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            color: Kirigami.Theme.backgroundColor

            Rectangle {
                anchors.top: parent.top
                width: parent.width
                height: 1
                color: "lightGray"
            }

            ColumnLayout {
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width

                Rectangle {
                    id: progressBar
                    Layout.fillWidth: parent
                    Layout.leftMargin: Kirigami.Units.largeSpacing
                    Layout.rightMargin: Kirigami.Units.largeSpacing
                    Layout.topMargin: Kirigami.Units.smallSpacing
                    radius: 5
                    height: 5

                    color: "gray"

                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        radius: 5

                        width: mediaPlayer.duration > 0 ? parent.width*mediaPlayer.position/mediaPlayer.duration : 0

                        color: {
                            Kirigami.Theme.colorSet = Kirigami.Theme.View
                            var accentColor = Kirigami.Theme.highlightColor
                            return Qt.tint(Kirigami.Theme.backgroundColor, Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 1))
                        }
                    }

                    MouseArea {
                        anchors.fill: parent

                        onClicked: {
                            if (mediaPlayer.seekable) {
                                mediaPlayer.seek(mediaPlayer.duration * mouse.x/width);
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: Kirigami.Units.largeSpacing

                    Button {
                        id: backwardButton
                        icon.name: "media-seek-backward"

                        onClicked: {
                            if (mediaPlayer.seekable) {
                                mediaPlayer.seek(mediaPlayer.position - 2000)
                            }
                        }
                    }

                    Button {
                        id: playAndPauseButton
                        icon.name: "media-playback-pause"

                        onClicked: {
                            if (icon.name == "media-playback-start") {
                                mediaPlayer.play()
                                icon.name = "media-playback-pause"
                            } else {
                                mediaPlayer.pause()
                                icon.name = "media-playback-start"
                            }
                        }
                    }

                    Button {
                        id: forwardButton
                        icon.name: "media-seek-forward"

                        onClicked: {
                            if (mediaPlayer.seekable) {
                                mediaPlayer.seek(mediaPlayer.position + 2000)
                            }
                        }
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        mediaPlayer.play()
    }
}
