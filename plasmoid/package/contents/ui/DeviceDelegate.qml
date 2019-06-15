/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
            Item {
                //spacer to make the label centre aligned in a row yet still elide and everything
                implicitWidth: (ring.visible? ring.width : 0) + (browse.visible? browse.width : 0) + (shareFile.visible? shareFile.width : 0) + parent.spacing
            }

            Battery {
                id: battery
                device: root.device
            }

            PlasmaComponents.Label {
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                text: (battery.available && battery.charge > -1) ? i18n("%1 (%2)", display, battery.displayString) : display
                Layout.fillWidth: true
                textFormat: Text.PlainText
            }

            //Share
            PlasmaComponents.Button
            {
                FileDialog {
                    id: fileDialog
                    title: i18n("Please choose a file")
                    folder: shortcuts.home
                }

                id: shareFile
                iconSource: "document-share"
                visible: share.available
                tooltip: i18n("Share file")
                onClicked: {
                    fileDialog.open()
                    share.plugin.shareUrl(fileDialog.fileUrl)
                }
            }

            //Find my phone
            PlasmaComponents.Button
            {
                FindMyPhone {
                    id: findmyphone
                    device: root.device
                }

                id: ring
                iconSource: "irc-voice"
                visible: findmyphone.available
                tooltip: i18n("Ring my phone")

                onClicked: {
                    findmyphone.ring()
                }
            }

            //SFTP
            PlasmaComponents.Button
            {
                Sftp {
                    id: sftp
                    device: root.device
                }

                id: browse
                iconSource: "document-open-folder"
                visible: sftp.available
                tooltip: i18n("Browse this device")

                onClicked: {
                    sftp.browse()
                }
            }

            height: browse.height
            width: parent.width
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
	
        PlasmaComponents.Label {
            text: i18n("Share text")
        }
        PlasmaComponents3.TextArea {
            id: shareText
            width: parent.width
        }
        PlasmaComponents.Button
        {
            id: submitTextButton
            anchors.right: shareText.right
            iconSource: "document-send"
            onClicked: {
                share.plugin.shareText(shareText.getText(0, shareText.length))
                shareText.clear()
            }
        }
        //NOTE: More information could be displayed here
    }
}
