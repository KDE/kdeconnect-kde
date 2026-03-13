/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 * SPDX-FileCopyrightText: 2024 ivan tkachenko <me@ratijas.tk>
 * SPDX-FileCopyrightText: 2026 Lito Parra <lito.15@proton.me>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Dialogs as QtDialogs
import QtQuick.Layouts

import org.kde.kdeconnect as KDEConnect
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.extras as PlasmaExtras

PlasmaComponents.ItemDelegate {
    id: root

    required property int index
    required property var model

    readonly property KDEConnect.DeviceDbusInterface device: KDEConnect.DeviceDbusInterfaceFactory.create(model.deviceId)

    property bool notificationsExpanded: false

    hoverEnabled: true
    down: false

    background: Item {}

    padding: Kirigami.Units.smallSpacing

    Battery {
        id: battery
        device: root.device
    }

    Clipboard {
        id: clipboard
        device: root.device
    }

    Connectivity {
        id: connectivity
        device: root.device
    }

    FindMyPhone {
        id: findmyphone
        device: root.device
    }

    RemoteCommands {
        id: remoteCommands
        device: root.device
    }

    Sftp {
        id: sftp
        device: root.device
    }

    Share {
        id: share
        device: root.device
    }

    SMS {
        id: sms
        device: root.device
    }

    VirtualMonitor {
        id: virtualmonitor
        device: root.device
    }

    Kirigami.PromptDialog {
        id: prompt
        visible: false
        showCloseButton: true
        standardButtons: Kirigami.Dialog.NoButton
        title: i18n("Virtual Monitor is not available")
    }

    QtDialogs.FileDialog {
        id: fileDialog
        title: i18n("Please choose a file")
        currentFolder: StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]
        modality: Qt.NonModal
        fileMode: QtDialogs.FileDialog.OpenFiles
        onAccepted: share.plugin.shareUrls(fileDialog.selectedFiles)
    }

    PlasmaExtras.Menu {
        id: menu

        visualParent: overflowMenu
        placement: PlasmaExtras.Menu.BottomPosedLeftAlignedPopup

        // Share
        PlasmaExtras.MenuItem {
            icon: "document-share"
            visible: share.available
            text: i18n("Share file")
            onClicked: fileDialog.open()
        }

        // Clipboard
        PlasmaExtras.MenuItem {
            icon: "klipper"
            visible: clipboard.clipboard?.isAutoShareDisabled ?? false
            text: i18n("Send Clipboard")

            onClicked: {
                clipboard.sendClipboard()
            }
        }

        // Find my phone
        PlasmaExtras.MenuItem {
            icon: "irc-voice"
            visible: findmyphone.available
            text: i18n("Ring my phone")

            onClicked: {
                findmyphone.ring()
            }
        }

        // SFTP
        PlasmaExtras.MenuItem {
            icon: "document-open-folder"
            visible: sftp.available
            text: i18n("Browse this device")

            onClicked: {
                sftp.browse()
            }
        }

        // SMS
        PlasmaExtras.MenuItem {
            icon: "message-new"
            visible: sms.available
            text: i18n("SMS Messages")

            onClicked: {
                sms.plugin.launchApp()
            }
        }
    }

    DropArea {
        id: fileDropArea
        anchors.fill: parent

        onDropped: drop => {
            if (drop.hasUrls) {
                const urls = new Set(drop.urls.map(url => url.toString()));
                urls.forEach(url => share.plugin.shareUrl(url));
            }
            drop.accepted = true;
        }
    }

    contentItem: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        PlasmaComponents.ItemDelegate {
            id: deviceHeaderContainer
            hoverEnabled: true
            padding: 0
            topPadding: Kirigami.Units.smallSpacing
            bottomPadding: topPadding

            Layout.fillWidth: true
            Layout.margins: 0

            background: Item {}

            // ToolTipArea covering entire header area
            PlasmaCore.ToolTipArea {
                anchors.fill: parent
                z: -1  // Behind content so it doesn't block interactions
                
                // Show only when hovering empty space (not hovering any interactive buttons)
                active: deviceHeaderContainer.hovered && !overflowMenu.hovered && !virtualMonitorButton.hovered && !dismissAllButton.hovered
                mainText: i18n("File Transfer")
                subText: i18n("Drop a file to transfer it onto your phone.")
            }

            // Real device header
            contentItem: ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignVCenter
                spacing: Kirigami.Units.smallSpacing

                Layout.margins: 0

                RowLayout {
                    // Device name
                    Kirigami.Heading {
                        text: root.model.name
                        level: 2
                        elide: Text.ElideRight
                        maximumLineCount: 1

                        leftPadding: LayoutMirroring.enabled ? 0 : Kirigami.Units.smallSpacing
                        rightPadding: LayoutMirroring.enabled ? Kirigami.Units.smallSpacing : 0
                    }

                    // Spacer - pushes indicators/buttons to the right. Explicitly non-interactive.
                    Item {
                        Layout.fillWidth: true
                        enabled: false  // Will not capture mouse/hover/drop events
                    }

                    // Remote screen icon
                    PlasmaComponents.ToolButton {
                        id: virtualMonitorButton
                        icon.name: "krdc"
                        icon.width: Kirigami.Units.iconSizes.small
                        icon.height: icon.width
                        visible: virtualmonitor.available
                        display: PlasmaComponents.AbstractButton.IconOnly
                        checked: visible && virtualmonitor.plugin.active
                        checkable: true
                        
                        hoverEnabled: true
                        PlasmaComponents.ToolTip.text: i18n("Virtual Display")

                        onClicked: {
                            if (virtualmonitor.plugin.active) {
                                virtualmonitor.plugin.stop();
                                prompt.visible = false;
                            } else {
                                virtualmonitor.plugin.requestVirtualMonitor();
                                prompt.subtitle = virtualmonitor.plugin.lastError;
                                prompt.visible = prompt.subtitle.length > 0;
                            }
                        }
                    }

                    RowLayout {
                        id: connectionInformation

                        visible: connectivity.available
                        spacing: Kirigami.Units.smallSpacing

                        // TODO: In the future, when the Connectivity Report plugin supports more than one
                        // subscription, add more signal strength icons to represent all the available
                        // connections.

                        Kirigami.Icon {
                            id: celluarConnectionStrengthIcon
                            source: connectivity.iconName
                            Layout.preferredHeight: Kirigami.Units.iconSizes.small
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

                    RowLayout {
                        id: batteryInformation

                        visible: battery.available && battery.hasBattery
                        spacing: Kirigami.Units.smallSpacing

                        Kirigami.Icon {
                            id: batteryIcon
                            source: battery.iconName
                            Layout.preferredHeight: Kirigami.Units.iconSizes.small
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
                        Layout.preferredHeight: Kirigami.Units.iconSizes.medium
                        Layout.preferredWidth: Layout.preferredHeight

                        highlighted: hovered

                        PlasmaComponents.ToolTip.text: i18n("More options…")

                        onClicked: menu.openRelative()
                    }

                }

                // Notifications Header
                RowLayout {
                    visible: notificationsModel.count > 0
                    Layout.fillWidth: true
                    Layout.margins: 0
                    spacing: Kirigami.Units.smallSpacing

                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.notificationsExpanded = !root.notificationsExpanded
                        cursorShape: Qt.PointingHandCursor
                    }

                    // Collapse/Expand indicator - clickable
                    Kirigami.Icon {
                        id: collapseIcon
                        source: "arrow-down"
                        Layout.preferredHeight: Kirigami.Units.iconSizes.small
                        Layout.preferredWidth: Layout.preferredHeight
                        color: Kirigami.Theme.textColor

                        rotation: root.notificationsExpanded ? 180 : 0

                        Behavior on rotation {
                            NumberAnimation {
                                duration: Kirigami.Units.shortDuration
                                easing.type: Easing.InOutQuad
                            }
                        }
                    }

                    // Collapsable list tab marker
                    PlasmaComponents.Label {
                        text: i18n("Notifications")
                        Layout.fillWidth: true
                    }

                    PlasmaComponents.ToolButton {
                        id: dismissAllButton
                        enabled: true
                        visible: notificationsModel.isAnyDimissable;
                        icon.name: "edit-clear-history"
                        icon.width: Kirigami.Units.iconSizes.smallMedium
                        icon.height: icon.width

                        PlasmaComponents.ToolTip.text: i18n("Dismiss all notifications")
                        onClicked: dismissAllAnimation.start();
                    }
                }
            }
        }

        // RemoteKeyboard
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

                KDEConnect.RemoteKeyboard {
                    id: remoteKeyboard
                    device: root.device
                    Layout.fillWidth: true
                }
            }
        }

        Item {
            id: notificationsContainer
            Layout.fillWidth: true
            Layout.leftMargin: 0
            Layout.rightMargin: 0
            Layout.bottomMargin: 0
            Layout.topMargin: Kirigami.Units.smallSpacing
            Layout.preferredHeight: root.notificationsExpanded ? notificationsColumn.implicitHeight : 0
            clip: true
            visible: notificationsModel.count > 0

            Behavior on Layout.preferredHeight {
                NumberAnimation {
                    duration: Kirigami.Units.shortDuration
                    easing.type: Easing.InOutQuad
                }
            }

            // Dismiss all animation - slides and fades out the entire container
            SequentialAnimation {
                id: dismissAllAnimation
                NumberAnimation { 
                    target: notificationsColumn
                    property: "x"
                    to: LayoutMirroring.enabled ? -notificationsContainer.width : notificationsContainer.width
                    duration: Kirigami.Units.shortDuration
                }
                ScriptAction { 
                    script: {
                        notificationsModel.dismissAll();
                    }
                }
                PauseAnimation {
                    duration: 50 // Small delay to ensure model update propagates
                }
                ScriptAction {
                    script: {
                        // Only reset if all notifications are gone
                        if (notificationsModel.count === 0) {
                            notificationsColumn.x = 0;
                        }
                    }
                }
            }

            Column {
                id: notificationsColumn
                width: parent.width

                // Animate items moving to new positions when others are removed
                move: Transition {
                    NumberAnimation {
                        properties: "y"
                        duration: Kirigami.Units.shortDuration
                        easing.type: Easing.OutInQuad
                    }
                }

                // Smooth fade in for new items
                add: Transition {
                    NumberAnimation {
                        properties: "opacity"
                        from: 0
                        to: 1
                        duration: Kirigami.Units.shortDuration
                    }
                }

                Repeater {
                    id: notificationsView

                    model: KDEConnect.NotificationsModel {
                        id: notificationsModel
                        deviceId: root.model.deviceId
                    }

                    // Notification item
                    delegate: PlasmaComponents.ItemDelegate {
                        id: listitem

                        required property int index
                        required property var model

                        hoverEnabled: true
                        width: parent.width
                        height: implicitHeight

                        clip: true

                        // Control animation suppression (for instant collapsing)
                        property bool _suppressAnimations: false

                        topPadding: index != 0 ? Kirigami.Units.mediumSpacing * 2 : 0
                        bottomPadding: index != notificationsModel.count - 1 ? Kirigami.Units.mediumSpacing * 2 : 0
                        leftPadding: LayoutMirroring.enabled ? 0 : Kirigami.Units.smallSpacing
                        rightPadding: LayoutMirroring.enabled ? Kirigami.Units.smallSpacing : 0

                        Layout.margins: 0

                        Behavior on height {
                            NumberAnimation {
                                duration: Kirigami.Units.shortDuration
                                easing.type: Easing.InOutQuad
                            }
                        }

                        // Dismiss with animation
                        SequentialAnimation {
                            id: dismissAnimation
                            NumberAnimation { target: listitem; property: "x"; to: LayoutMirroring.enabled ? -width : width; duration: Kirigami.Units.shortDuration }
                            ScriptAction { script: listitem.model.dbusInterface.dismiss() }
                        }

                        Behavior on height {
                            enabled: !listitem._suppressAnimations
                            NumberAnimation {
                                duration: Kirigami.Units.shortDuration
                                easing.type: Easing.InOutQuad
                            }
                        }

                        background: Item {}

                        Kirigami.Theme.colorSet: Kirigami.Theme.View
                        Kirigami.Theme.inherit: false

                        enabled: true
                        onClicked: checked = !checked

                        // Timer to delay collapse until list animation finishes
                        Timer {
                            id: collapseDelayTimer
                            interval: Kirigami.Units.shortDuration  // Match the list collapse duration
                            onTriggered: {
                                listitem._suppressAnimations = true
                                listitem.checked = false
                                listitem._suppressAnimations = false
                            }
                        }

                        // Collapse if notification list is collapsed
                        // Watch for list expansion state changes
                        Connections {
                            target: root
                            function onNotificationsExpandedChanged() {
                                if (!root.notificationsExpanded) {
                                    collapseDelayTimer.start()
                                } else {
                                    collapseDelayTimer.stop()
                                }
                            }
                        }

                        property bool replying: false

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
                                                    level: 5
                                                    elide: listitem.checked ? Text.ElideNone : Text.ElideRight
                                                    maximumLineCount: listitem.checked ? 0 : 1
                                                    wrapMode: Text.Wrap
                                                    Layout.alignment: Qt.AlignVCenter
                                                    Layout.fillWidth: true

                                                    color: Kirigami.ColorUtils.tintWithAlpha(
                                                        Kirigami.Theme.backgroundColor,
                                                        Kirigami.Theme.textColor,
                                                        0.85
                                                    )
                                                }
                                            }

                                            // Notification title
                                            Kirigami.Heading {
                                                id: notificationTitle
                                                text: listitem.model.title
                                                level: 2
                                                type: Kirigami.Heading.Type.Primary
                                                visible: text.length > 0
                                                elide: listitem.checked ? Text.ElideNone : Text.ElideRight
                                                maximumLineCount: listitem.checked ? 0 : 1
                                                wrapMode: Text.Wrap
                                                Layout.fillWidth: true
                                            }
                                        }

                                        PlasmaComponents.ToolButton {
                                            id: replyButton
                                            visible: listitem.model.repliable
                                            enabled: listitem.model.repliable && !listitem.replying
                                            icon.name: "mail-reply-sender"
                                            icon.width: Kirigami.Units.iconSizes.smallMedium
                                            icon.height: icon.width
                                            PlasmaComponents.ToolTip.text: i18n("Reply")
                                            onClicked: {
                                                listitem.replying = true;
                                                replyTextField.forceActiveFocus();
                                            }

                                            Layout.alignment: Qt.AlignTop
                                        }

                                        PlasmaComponents.ToolButton {
                                            id: dismissButton
                                            visible: notificationsModel.isAnyDimissable;
                                            enabled: listitem.model.dismissable
                                            Layout.alignment: Qt.AlignTop
                                            icon.name: "window-close"
                                            icon.width: Kirigami.Units.iconSizes.smallMedium
                                            icon.height: icon.width
                                            PlasmaComponents.ToolTip.text: i18n("Dismiss")
                                            onClicked: dismissAnimation.start()
                                        }
                                    }

                                    PlasmaComponents.Label {
                                        id: notificationNotitext
                                        text: listitem.model.notitext
                                        visible: text.length > 0
                                        elide: listitem.checked ? Text.ElideNone : Text.ElideRight
                                        maximumLineCount: listitem.checked ? 0 : 1
                                        wrapMode: Text.Wrap
                                        Layout.fillWidth: true
                                    }
                                }
                            }

                            RowLayout {
                                visible: listitem.replying
                                spacing: Kirigami.Units.smallSpacing
                                Layout.topMargin: Kirigami.Units.smallSpacing
                                Layout.fillWidth: true

                                PlasmaComponents.ToolButton {
                                    id: replyCancelButton
                                    Layout.alignment: Qt.AlignBottom
                                    text: i18n("Cancel")
                                    padding: replyTextField.padding
                                    display: PlasmaComponents.AbstractButton.IconOnly
                                    PlasmaComponents.ToolTip {
                                        text: replyCancelButton.text
                                    }
                                    icon.name: "dialog-cancel"
                                    onClicked: {
                                        replyTextField.text = "";
                                        listitem.replying = false;
                                    }
                                }

                                PlasmaComponents.TextArea {
                                    id: replyTextField
                                    placeholderText: i18nc("@info:placeholder", "Reply to %1…", listitem.model.appName)
                                    wrapMode: TextEdit.Wrap
                                    Layout.fillWidth: true
                                    Keys.onPressed: event => {
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

                                PlasmaComponents.ToolButton {
                                    Layout.alignment: Qt.AlignBottom
                                    id: replySendButton
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

                        Kirigami.Separator {
                            anchors.bottom: parent.bottom
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: parent.width - deviceHeaderContainer.leftPadding - deviceHeaderContainer.rightPadding
                            opacity: .5
                            visible: index < notificationsModel.count - 1
                        }
                    }
                }
            }
        }

        // Commands
        RowLayout {
            visible: remoteCommands.available && commandsModel.count > 0
            width: parent.width
            spacing: Kirigami.Units.smallSpacing

            PlasmaComponents.Label {
                text: i18n("Run command")
                Layout.fillWidth: true
            }

            PlasmaComponents.Button {
                id: addCommandButton
                icon.name: "list-add"
                PlasmaComponents.ToolTip.text: i18n("Add command")
                onClicked: remoteCommands.plugin.editCommands()
                visible: remoteCommands.plugin?.canAddCommand ?? false
            }
        }

        Repeater {
            id: commandsView

            visible: remoteCommands.available

            model: KDEConnect.RemoteCommandsModel {
                id: commandsModel
                deviceId: root.model.deviceId
            }

            delegate: PlasmaComponents.ItemDelegate {
                id: commandDelegate

                required property int index
                required property var model

                enabled: true

                onClicked: {
                    remoteCommands.plugin?.triggerCommand(commandDelegate.model.key);
                }

                Layout.fillWidth: true

                contentItem: PlasmaComponents.Label {
                    text: `${commandDelegate.model.name}\n${commandDelegate.model.command}`
                }
            }
        }
    }
}
