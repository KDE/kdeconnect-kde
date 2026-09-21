/*
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kdeconnect
import org.kde.kdeconnect as KDEConnect
import org.kde.kirigami as Kirigami
import org.kde.kitemmodels as KItemModels

Kirigami.ScrollablePage {
    id: page

    required property var pluginInterface
    required property var device

    title: i18nc("@title:window", "Notifications")
    Component.onCompleted: {
        console.log("device =", device.id());
        console.log("pluginInterface =", pluginInterface);
    }

    Column {
        Repeater {
            id: notificationsView

            model: KDEConnect.NotificationsModel {
                id: notificationsModel

                deviceId: device.id()
            }

            // Notification item
            delegate: ItemDelegate {
                id: listitem

                required property int index
                required property var model
                property bool replying: false

                hoverEnabled: true
                width: parent.width
                height: implicitHeight
                clip: true
                topPadding: index != 0 ? Kirigami.Units.mediumSpacing * 2 : 0
                bottomPadding: index != notificationsModel.count - 1 ? Kirigami.Units.mediumSpacing * 2 : 0
                leftPadding: LayoutMirroring.enabled ? 0 : Kirigami.Units.smallSpacing
                rightPadding: LayoutMirroring.enabled ? Kirigami.Units.smallSpacing : 0
                Layout.margins: 0
                Kirigami.Theme.colorSet: Kirigami.Theme.View
                Kirigami.Theme.inherit: false
                enabled: true

                // Dismiss with animation
                SequentialAnimation {
                    id: dismissAnimation

                    NumberAnimation {
                        target: listitem
                        property: "x"
                        to: listitem.LayoutMirroring.enabled ? -width : width
                        duration: Kirigami.Units.shortDuration
                    }

                    ScriptAction {
                        script: listitem.model.dbusInterface.dismiss()
                    }

                }

                Kirigami.Separator {
                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: parent.width
                    opacity: 0.5
                    visible: index < notificationsModel.count - 1
                }

                Behavior on height {
                    NumberAnimation {
                        duration: Kirigami.Units.shortDuration
                        easing.type: Easing.InOutQuad
                    }

                }

                background: Item {
                }

                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing

                    RowLayout {
                        spacing: Kirigami.Units.largeSpacing
                        Layout.alignment: Qt.AlignTop

                        ColumnLayout {
                            Layout.alignment: Qt.AlignTop

                            RowLayout {
                                id: notificationHead

                                spacing: Kirigami.Units.largeSpacing
                                Layout.alignment: Qt.AlignTop

                                ColumnLayout {
                                    // Couple together so Dismiss and Reply have enough room
                                    spacing: Kirigami.Units.smallSpacing

                                    // Notification and icon
                                    RowLayout {
                                        Layout.alignment: Qt.AlignVCenter

                                        Kirigami.Icon {
                                            id: notificationIcon

                                            source: listitem.model.appIcon
                                            Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                                            Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
                                            Layout.alignment: Qt.AlignVCenter
                                        }

                                        Kirigami.Heading {
                                            id: notificationAppName

                                            text: listitem.model.appName
                                            textFormat: Text.PlainText
                                            level: 5
                                            elide: Text.ElideRight
                                            maximumLineCount: 1
                                            wrapMode: Text.Wrap
                                            Layout.alignment: Qt.AlignVCenter
                                            Layout.fillWidth: true
                                            color: Kirigami.ColorUtils.tintWithAlpha(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, 0.85)
                                        }

                                    }

                                    // Notification title
                                    Kirigami.SelectableLabel {
                                        text: listitem.model.title
                                        textFormat: TextEdit.PlainText
                                        visible: text.length > 0
                                        font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.2
                                        font.weight: Font.DemiBold
                                        wrapMode: TextEdit.Wrap
                                        Layout.fillWidth: true
                                    }

                                }

                                ToolButton {
                                    id: replyButton

                                    visible: listitem.model.repliable
                                    enabled: listitem.model.repliable && !listitem.replying
                                    icon.name: "mail-reply-sender"
                                    icon.width: Kirigami.Units.iconSizes.smallMedium
                                    icon.height: Kirigami.Units.iconSizes.smallMedium
                                    ToolTip.text: i18n("Reply")
                                    onClicked: {
                                        listitem.replying = true;
                                        replyTextField.forceActiveFocus();
                                    }
                                    Layout.alignment: Qt.AlignTop
                                }

                                ToolButton {
                                    id: dismissButton

                                    visible: notificationsModel.isAnyDimissable
                                    enabled: listitem.model.dismissable
                                    Layout.alignment: Qt.AlignTop
                                    icon.name: "window-close"
                                    icon.width: Kirigami.Units.iconSizes.smallMedium
                                    icon.height: Kirigami.Units.iconSizes.smallMedium
                                    ToolTip.text: i18n("Dismiss")
                                    onClicked: dismissAnimation.start()
                                }

                            }

                            Kirigami.SelectableLabel {
                                text: listitem.model.notitext
                                visible: text.length > 0
                                wrapMode: TextEdit.Wrap
                                Layout.fillWidth: true
                            }

                        }

                    }

                    RowLayout {
                        visible: listitem.replying
                        spacing: Kirigami.Units.smallSpacing
                        Layout.topMargin: Kirigami.Units.smallSpacing
                        Layout.fillWidth: true

                        ToolButton {
                            id: replyCancelButton

                            Layout.alignment: Qt.AlignBottom
                            text: i18n("Cancel")
                            padding: replyTextField.padding
                            display: AbstractButton.IconOnly
                            icon.name: "dialog-cancel"
                            onClicked: {
                                replyTextField.text = "";
                                listitem.replying = false;
                            }

                            ToolTip {
                                text: replyCancelButton.text
                            }

                        }

                        TextArea {
                            id: replyTextField

                            placeholderText: i18nc("@info:placeholder", "Reply to %1…", listitem.model.appName)
                            wrapMode: TextEdit.Wrap
                            Layout.fillWidth: true
                            Keys.onPressed: (event) => {
                                if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && !(event.modifiers & Qt.ShiftModifier)) {
                                    replySendButton.clicked();
                                    event.accepted = true;
                                }
                                if (event.key === Qt.Key_Escape) {
                                    replyCancelButton.clicked();
                                    event.accepted = true;
                                }
                            }
                        }

                        ToolButton {
                            id: replySendButton

                            Layout.alignment: Qt.AlignBottom
                            text: i18n("Send")
                            padding: replyTextField.padding
                            icon.name: LayoutMirroring.enabled ? "document-send-rtl" : "document-send"
                            enabled: replyTextField.text !== ""
                            onClicked: {
                                listitem.model.dbusInterface.sendReply(replyTextField.text);
                                replyTextField.text = "";
                                listitem.replying = false;
                            }
                        }

                    }

                }

            }

        }

    }

}
