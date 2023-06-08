/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.1
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kdeconnect 1.0
import QtQuick.Dialogs
import QtQuick.Controls 2.4

PlasmaComponents3.ItemDelegate
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

    Column {
        width: parent.width

        RowLayout
        {

            width: parent.width
            Battery {
                id: battery
                device: root.device
            }

            Connectivity {
                id: connectivity
                device: root.device
            }

            PlasmaComponents3.Label {
                id: deviceName
                elide: Text.ElideRight
                text: model.name
                Layout.fillWidth: true
                textFormat: Text.PlainText
            }

            PlasmaComponents3.ToolButton {
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

                // TODO: In the future, when the Connectivity Report plugin supports more than one
                // subscription, add more signal strength icons to represent all the available
                // connections.

                PlasmaCore.IconItem {
                    id: celluarConnectionStrengthIcon
                    source: connectivity.iconName
                    Layout.preferredHeight: connectivityText.height
                    Layout.preferredWidth: Layout.preferredHeight
                    Layout.alignment: Qt.AlignCenter
                    visible: valid
                }

                PlasmaComponents3.Label {
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

                PlasmaCore.IconItem {
                    id: batteryIcon
                    source: battery.iconName
                    // Make the icon the same size as the text so that it doesn't dominate
                    Layout.preferredHeight: batteryPercent.height
                    Layout.preferredWidth: Layout.preferredHeight
                    Layout.alignment: Qt.AlignCenter
                }

                PlasmaComponents3.Label {
                    id: batteryPercent
                    text: i18nc("Battery charge percentage", "%1%", battery.charge)
                    textFormat: Text.PlainText
                }
            }

            PlasmaComponents3.ToolButton {
                id: overflowMenu
                icon.name: "application-menu"
                checked: menu.status === PlasmaExtras.DialogStatus.Open

                onPressed: menu.openRelative()

                PlasmaExtras.Menu {
                    id: menu
                    visualParent: overflowMenu
                    placement: PlasmaCore.Types.BottomPosedLeftAlignedPopup

                    //Share
                    PlasmaExtras.MenuItem
                    {
                        FileDialog {
                            id: fileDialog
                            title: i18n("Please choose a file")
                            currentFolder: shortcuts.home
                            fileMode: FileDialog.OpenFiles
                            onAccepted: fileDialog.fileUrls.forEach(url => share.plugin.shareUrl(url))
                        }

                        id: shareFile
                        icon: "document-share"
                        visible: share.available
                        text: i18n("Share file")
                        onClicked: fileDialog.open()
                    }

                    //Photo
                    PlasmaExtras.MenuItem
                    {
                        FileDialog {
                            id: photoFileDialog
                            title: i18n("Save As")
                            currentFolder: shortcuts.pictures
                            fileMode: FileDialog.SaveFile
                            //selectExisting: false
                            onAccepted: {
                                var path = photoFileDialog.fileUrl.toString();
                                photo.plugin.requestPhoto(path);
                            }
                        }

                        id: takePhoto
                        icon: "camera-photo-symbolic"
                        visible: photo.available
                        text: i18n("Take a photo")
                        onClicked: photoFileDialog.open()
                    }

                    //Find my phone
                    PlasmaExtras.MenuItem
                    {
                        FindMyPhone {
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
                        Sftp {
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
                        SMS {
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
        PlasmaComponents3.ItemDelegate {
            visible: remoteKeyboard.remoteState
            width: parent.width

            RowLayout {
                width: parent.width
                spacing: 5

                PlasmaComponents3.Label {
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
        PlasmaComponents3.ItemDelegate {
            visible: notificationsModel.count>0
            enabled: true
            PlasmaComponents3.Label {
                text: i18n("Notifications:")
            }
            PlasmaComponents3.ToolButton {
                enabled: true
                visible: notificationsModel.isAnyDimissable;
                anchors.right: parent.right
                icon.name: "edit-clear-history"
                PlasmaComponents3.ToolTip.text: i18n("Dismiss all notifications")
                PlasmaComponents3.ToolTip.visible: hovered
                PlasmaComponents3.ToolTip.delay: Kirigami.Units.toolTipDelay
                onClicked: notificationsModel.dismissAll();
            }
        }
        Repeater {
            id: notificationsView
            model: NotificationsModel {
                id: notificationsModel
                deviceId: root.device.id()
            }
            delegate: PlasmaComponents3.ItemDelegate {
                id: listitem
                enabled: true
                onClicked: checked = !checked

                property bool replying: false

                PlasmaCore.IconItem {
                    id: notificationIcon
                    source: appIcon
                    width: (valid && appIcon.length) ? dismissButton.width : 0
                    height: width
                    anchors.left: parent.left
                }
                PlasmaComponents3.Label {
                    id: notificationLabel
                    text: appName + ": " + (title.length>0 ? (appName==title?notitext:title+": "+notitext) : model.name)
                    anchors.right: replyButton.left
                    anchors.left: notificationIcon.right
                    elide: listitem.checked ? Text.ElideNone : Text.ElideRight
                    maximumLineCount: listitem.checked ? 0 : 1
                    wrapMode: Text.WordWrap
                }
                PlasmaComponents3.ToolButton {
                    id: replyButton
                    visible: repliable
                    enabled: repliable && !replying
                    anchors.right: dismissButton.left
                    icon.name: "mail-reply-sender"
                    PlasmaComponents3.ToolTip.text: i18n("Reply")
                    PlasmaComponents3.ToolTip.visible: hovered
                    PlasmaComponents3.ToolTip.delay: Kirigami.Units.toolTipDelay
                    onClicked: { replying = true; replyTextField.forceActiveFocus(); }
                }
                PlasmaComponents3.ToolButton {
                    id: dismissButton
                    visible: notificationsModel.isAnyDimissable;
                    enabled: dismissable
                    anchors.right: parent.right
                    icon.name: "window-close"
                    PlasmaComponents3.ToolTip.text: i18n("Dismiss")
                    PlasmaComponents3.ToolTip.visible: hovered
                    PlasmaComponents3.ToolTip.delay: Kirigami.Units.toolTipDelay
                    onClicked: dbusInterface.dismiss();
                }
                RowLayout {
                    visible: replying
                    anchors.top: notificationLabel.bottom
                    anchors.left: notificationIcon.right
                    width: notificationLabel.width + replyButton.width + dismissButton.width
                    PlasmaComponents3.Button {
                        Layout.alignment: Qt.AlignBottom
                        id: replyCancelButton
                        text: i18n("Cancel")
                        display: PlasmaComponents3.AbstractButton.IconOnly
                        PlasmaComponents3.ToolTip {
                            text: parent.text
                        }
                        icon.name: "dialog-cancel"
                        onClicked: {
                            replyTextField.text = "";
                            replying = false;
                        }
                    }
                    PlasmaComponents3.TextArea {
                        id: replyTextField
                        placeholderText: i18nc("@info:placeholder", "Reply to %1…", appName)
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
                    PlasmaComponents3.Button {
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

        RemoteCommands {
            id: rc
            device: root.device
        }

        // Commands
        RowLayout {
            visible: rc.available
            width: parent.width

            PlasmaComponents3.Label {
                text: i18n("Run command")
                Layout.fillWidth: true
            }

            PlasmaComponents3.Button
            {
                id: addCommandButton
                icon.name: "list-add"
                PlasmaComponents3.ToolTip.text: i18n("Add command")
                PlasmaComponents3.ToolTip.visible: hovered
                PlasmaComponents3.ToolTip.delay: Kirigami.Units.toolTipDelay
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
            delegate: PlasmaComponents3.ItemDelegate {
                enabled: true
                onClicked: rc.plugin.triggerCommand(key)

                PlasmaComponents3.Label {
                    text: name + "\n" + command
                }
            }
        }

        // Share
        Share {
            id: share
            device: root.device
        }

        // Photo
        Photo {
            id: photo
            device: root.device
        }
    }
}
