/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.1
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.kdeconnect 1.0
import QtQuick.Dialogs 1.0
import QtQuick.Controls 2.4

PlasmaComponents.ListItem
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

            PlasmaComponents.Label {
                id: deviceName
                elide: Text.ElideRight
                text: display
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

                PlasmaCore.IconItem {
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

            PlasmaComponents3.ToolButton {
                id: overflowMenu
                icon.name: "application-menu"
                checked: menu.status === PlasmaComponents.DialogStatus.Open

                onPressed: menu.openRelative()

                PlasmaComponents.ContextMenu {
                    id: menu
                    visualParent: overflowMenu
                    placement: PlasmaCore.Types.BottomPosedLeftAlignedPopup

                    //Share
                    PlasmaComponents.MenuItem
                    {
                        FileDialog {
                            id: fileDialog
                            title: i18n("Please choose a file")
                            folder: shortcuts.home
                            selectMultiple: true
                            onAccepted: fileDialog.fileUrls.forEach(url => share.plugin.shareUrl(url))
                        }

                        id: shareFile
                        icon: "document-share"
                        visible: share.available
                        text: i18n("Share file")
                        onClicked: fileDialog.open()
                    }

                    //Find my phone
                    PlasmaComponents.MenuItem
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
                    PlasmaComponents.MenuItem
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
                    PlasmaComponents.MenuItem
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
        PlasmaComponents.ListItem {
            visible: remoteKeyboard.remoteState
            width: parent.width

            RowLayout {
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
        PlasmaComponents.ListItem {
            visible: notificationsModel.count>0
            enabled: true
            PlasmaComponents.Label {
                text: i18n("Notifications:")
            }
            PlasmaComponents.ToolButton {
                enabled: true
                visible: notificationsModel.isAnyDimissable;
                anchors.right: parent.right
                iconSource: "edit-clear-history"
                tooltip: i18n("Dismiss all notifications")
                onClicked: notificationsModel.dismissAll();
            }
        }
        Repeater {
            id: notificationsView
            model: NotificationsModel {
                id: notificationsModel
                deviceId: root.device.id()
            }
            delegate: PlasmaComponents.ListItem {
                id: listitem
                enabled: true
                onClicked: checked = !checked

                PlasmaCore.IconItem {
                    id: notificationIcon
                    source: appIcon
                    width: (valid && appIcon.length) ? dismissButton.width : 0
                    height: width
                    anchors.left: parent.left
                }
                PlasmaComponents.Label {
                    text: appName + ": " + (title.length>0 ? (appName==title?notitext:title+": "+notitext) : display)
                    anchors.right: replyButton.left
                    anchors.left: notificationIcon.right
                    elide: listitem.checked ? Text.ElideNone : Text.ElideRight
                    maximumLineCount: listitem.checked ? 0 : 1
                    wrapMode: Text.WordWrap
                }
                PlasmaComponents.ToolButton {
                    id: replyButton
                    visible: repliable
                    enabled: repliable
                    anchors.right: dismissButton.left
                    iconSource: "mail-reply-sender"
                    tooltip: i18n("Reply")
                    onClicked: dbusInterface.reply();
                }
                PlasmaComponents.ToolButton {
                    id: dismissButton
                    visible: notificationsModel.isAnyDimissable;
                    enabled: dismissable
                    anchors.right: parent.right
                    iconSource: "window-close"
                    tooltip: i18n("Dismiss")
                    onClicked: dbusInterface.dismiss();
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

            PlasmaComponents.Label {
                text: i18n("Run command")
                Layout.fillWidth: true
            }

            PlasmaComponents.Button
            {
                id: addCommandButton
                iconSource: "list-add"
                tooltip: i18n("Add command")
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
            delegate: PlasmaComponents.ListItem {
                enabled: true
                onClicked: rc.plugin.triggerCommand(key)

                PlasmaComponents.Label {
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
