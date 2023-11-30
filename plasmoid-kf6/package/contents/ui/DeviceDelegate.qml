/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Layouts
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PlasmaComponents
import org.kde.kdeconnect
import QtQuick.Controls
import org.kde.kirigami as Kirigami
import org.kde.plasma.extras as PlasmaExtras
import QtQuick.Dialogs
import QtCore

PlasmaComponents.ItemDelegate
{
    id: root
    readonly property QtObject device: DeviceDbusInterfaceFactory.create(model.deviceId)

    DropArea {
        id: fileDropArea
        anchors.fill: parent

        onDropped: {
            if (drop.hasUrls) {

                var urls = [];

                for (var v in drop.urls) {
                    if (drop.urls[v] != null) {
                        if (urls.indexOf(drop.urls[v].toString()) == -1) {
                            urls.push(drop.urls[v].toString());
                        }
                    }
                }

                var i;
                for (i = 0; i < urls.length; i++) {
                    share.plugin.shareUrl(urls[i]);
                }
            }
            drop.accepted = true;
        }

        PlasmaCore.ToolTipArea {
            id: dropAreaToolTip
            anchors.fill: parent
            location: plasmoid.location
            active: true
            mainText: i18n("File Transfer")
            subText: i18n("Drop a file to transfer it onto your phone.")
        }
    }

    contentItem: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        RowLayout
        {
            width: parent.width
            spacing: Kirigami.Units.smallSpacing

            Battery {
                id: battery
                device: root.device
            }

            Connectivity {
                id: connectivity
                device: root.device
            }

            PlasmaComponents.Label {
                id: deviceName
                elide: Text.ElideRight
                text: model.name
                Layout.fillWidth: true
                textFormat: Text.PlainText
            }

            PlasmaComponents.ToolButton {
                VirtualMonitor {
                    id: vd
                    device: root.device
                }
                icon.name: "video-monitor"
                text: i18n("Virtual Display")
                visible: vd.available
                onClicked: {
                    if (!vd.plugin.requestVirtualMonitor()) {
                        console.warn("Failed to create the virtual monitor")
                    }
                }
            }
            RowLayout
            {
                id: connectionInformation
                visible: connectivity.available
                spacing: Kirigami.Units.smallSpacing

                // TODO: In the future, when the Connectivity Report plugin supports more than one
                // subscription, add more signal strength icons to represent all the available
                // connections.

                Kirigami.Icon {
                    id: celluarConnectionStrengthIcon
                    source: connectivity.iconName
                    Layout.preferredHeight: connectivityText.height
                    Layout.preferredWidth: Layout.preferredHeight
                    Layout.alignment: Qt.AlignCenter
                    visible: valid
                }

                PlasmaComponents.Label {
                    // Fallback plain-text label. Only show this if the icon doesn't work.
                    id: connectivityText
                    text: connectivity.displayString
                    textFormat: Text.PlainText
                    visible: !celluarConnectionStrengthIcon.visible
                }
            }

            RowLayout
            {
                id: batteryInformation
                visible: (battery.available && battery.charge > -1)
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Icon {
                    id: batteryIcon
                    source: battery.iconName
                    // Make the icon the same size as the text so that it doesn't dominate
                    Layout.preferredHeight: batteryPercent.height
                    Layout.preferredWidth: Layout.preferredHeight
                    Layout.alignment: Qt.AlignCenter
                }

                PlasmaComponents.Label {
                    id: batteryPercent
                    text: i18nc("Battery charge percentage", "%1%", battery.charge)
                    textFormat: Text.PlainText
                }
            }

            PlasmaComponents.ToolButton {
                id: overflowMenu
                icon.name: "application-menu"
                checked: menu.status === PlasmaExtras.Menu.Open

                onPressed: menu.openRelative()

                PlasmaExtras.Menu {
                    id: menu
                    visualParent: overflowMenu
                    placement: PlasmaExtras.Menu.BottomPosedLeftAlignedPopup

                    //Share
                    PlasmaExtras.MenuItem
                    {
                        property FileDialog data: FileDialog {
                            id: fileDialog
                            title: i18n("Please choose a file")
                            currentFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
                            fileMode: FileDialog.OpenFiles
                            onAccepted: fileDialog.selectedFiles.forEach(url => share.plugin.shareUrl(url))
                        }

                        id: shareFile
                        icon: "document-share"
                        visible: share.available
                        text: i18n("Share file")
                        onClicked: fileDialog.open()
                    }

                    //Clipboard
                    PlasmaExtras.MenuItem
                    {
                        property Clipboard data: Clipboard {
                            id: clipboard
                            device: root.device
                        }

                        id: sendclipboard
                        icon: "klipper"
                        visible: clipboard.available && clipboard.clipboard.isAutoShareDisabled
                        text: i18n("Send Clipboard")

                        onClicked: {
                            clipboard.sendClipboard()
                        }
                    }


                    //Find my phone
                    PlasmaExtras.MenuItem
                    {
                        property FindMyPhone data: FindMyPhone {
                            id: findmyphone
                            device: root.device
                        }

                        id: ring
                        icon: "irc-voice"
                        visible: findmyphone.available
                        text: i18n("Ring my phone")

                        onClicked: {
                            findmyphone.ring()
                        }
                    }

                    //SFTP
                    PlasmaExtras.MenuItem
                    {
                        property Sftp data: Sftp {
                            id: sftp
                            device: root.device
                        }

                        id: browse
                        icon: "document-open-folder"
                        visible: sftp.available
                        text: i18n("Browse this device")

                        onClicked: {
                            sftp.browse()
                        }
                    }

                    //SMS
                    PlasmaExtras.MenuItem
                    {
                        property SMS data: SMS {
                            id: sms
                            device: root.device
                        }

                        icon: "message-new"
                        visible: sms.available
                        text: i18n("SMS Messages")

                        onClicked: {
                            sms.plugin.launchApp()
                        }
                    }
                }
            }
        }

        //RemoteKeyboard
        PlasmaComponents.ItemDelegate {
            visible: remoteKeyboard.remoteState
            Layout.fillWidth: true

            contentItem: RowLayout {
                width: parent.width
                spacing: 5

                PlasmaComponents.Label {
                    id: remoteKeyboardLabel
                    text: i18n("Remote Keyboard")
                }

                RemoteKeyboard {
                    id: remoteKeyboard
                    device: root.device
                    Layout.fillWidth: true
                }
            }
        }

        //Notifications
        PlasmaComponents.ItemDelegate {
            visible: notificationsModel.count>0
            enabled: true
            Layout.fillWidth: true

            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                PlasmaComponents.Label {
                    text: i18n("Notifications:")
                }

                PlasmaComponents.ToolButton {
                    enabled: true
                    visible: notificationsModel.isAnyDimissable;
                    Layout.alignment: Qt.AlignRight
                    icon.name: "edit-clear-history"
                    ToolTip.text: i18n("Dismiss all notifications")
                    onClicked: notificationsModel.dismissAll();
                }
            }
        }
        Repeater {
            id: notificationsView
            model: NotificationsModel {
                id: notificationsModel
                deviceId: root.device.id()
            }
            delegate: PlasmaComponents.ItemDelegate {
                id: listitem
                enabled: true
                onClicked: checked = !checked
                Layout.fillWidth: true

                property bool replying: false

                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing

                    RowLayout {
                        spacing: Kirigami.Units.smallSpacing

                        Kirigami.Icon {
                            id: notificationIcon
                            source: appIcon
                            width: (valid && appIcon.length) ? dismissButton.width : 0
                            height: width
                            Layout.alignment: Qt.AlignLeft
                        }

                        PlasmaComponents.Label {
                            id: notificationLabel
                            text: appName + ": " + (title.length>0 ? (appName==title?notitext:title+": "+notitext) : model.name)
                            elide: listitem.checked ? Text.ElideNone : Text.ElideRight
                            maximumLineCount: listitem.checked ? 0 : 1
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        PlasmaComponents.ToolButton {
                            id: replyButton
                            visible: repliable
                            enabled: repliable && !replying
                            icon.name: "mail-reply-sender"
                            ToolTip.text: i18n("Reply")
                            onClicked: { replying = true; replyTextField.forceActiveFocus(); }
                        }

                        PlasmaComponents.ToolButton {
                            id: dismissButton
                            visible: notificationsModel.isAnyDimissable;
                            enabled: dismissable
                            Layout.alignment: Qt.AlignRight
                            icon.name: "window-close"
                            ToolTip.text: i18n("Dismiss")
                            onClicked: dbusInterface.dismiss();
                        }
                    }

                    RowLayout {
                        visible: replying
                        width: notificationLabel.width + replyButton.width + dismissButton.width + Kirigami.Units.smallSpacing * 2
                        spacing: Kirigami.Units.smallSpacing

                        PlasmaComponents.Button {
                            Layout.alignment: Qt.AlignBottom
                            id: replyCancelButton
                            text: i18n("Cancel")
                            display: PlasmaComponents.AbstractButton.IconOnly
                            PlasmaComponents.ToolTip {
                                text: parent.text
                            }
                            icon.name: "dialog-cancel"
                            onClicked: {
                                replyTextField.text = "";
                                replying = false;
                            }
                        }

                        PlasmaComponents.TextArea {
                            id: replyTextField
                            placeholderText: i18nc("@info:placeholder", "Reply to %1…", appName)
                            wrapMode: TextEdit.Wrap
                            Layout.fillWidth: true
                            Keys.onPressed: {
                                if ((event.key == Qt.Key_Return || event.key == Qt.Key_Enter) && !(event.modifiers & Qt.ShiftModifier)) {
                                    replySendButton.clicked();
                                    event.accepted = true;
                                }
                                if (event.key == Qt.Key_Escape) {
                                    replyCancelButton.clicked();
                                    event.accepted = true;
                                }
                            }
                        }

                        PlasmaComponents.Button {
                            Layout.alignment: Qt.AlignBottom
                            id: replySendButton
                            text: i18n("Send")
                            icon.name: "document-send"
                            enabled: replyTextField.text
                            onClicked: {
                                dbusInterface.sendReply(replyTextField.text);
                                replyTextField.text = "";
                                replying = false;
                            }
                        }
                    }
                }
            }
        }

        RemoteCommands {
            id: rc
            device: root.device
        }

        // Commands
        RowLayout {
            visible: rc.available
            width: parent.width
            spacing: Kirigami.Units.smallSpacing

            PlasmaComponents.Label {
                text: i18n("Run command")
                Layout.fillWidth: true
            }

            PlasmaComponents.Button
            {
                id: addCommandButton
                icon.name: "list-add"
                ToolTip.text: i18n("Add command")
                onClicked: rc.plugin.editCommands()
                visible: rc.plugin && rc.plugin.canAddCommand
            }
        }
        Repeater {
            id: commandsView
            visible: rc.available
            model: RemoteCommandsModel {
                id: commandsModel
                deviceId: rc.device.id()
            }
            delegate: PlasmaComponents.ItemDelegate {
                enabled: true
                onClicked: rc.plugin.triggerCommand(key)
                Layout.fillWidth: true

                contentItem: PlasmaComponents.Label {
                    text: name + "\n" + command
                }
            }
        }

        // Share
        Share {
            id: share
            device: root.device
        }
    }
}
