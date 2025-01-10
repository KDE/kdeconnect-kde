/*
 * SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kdeconnect
import org.kde.config as KConfig

Kirigami.ApplicationWindow {
    id: root
    property int columnWidth: Kirigami.Units.gridUnit * 13
    minimumWidth: Kirigami.Units.gridUnit * 15
    minimumHeight: Kirigami.Units.gridUnit * 15
    wideScreen: width > columnWidth * 5
    pageStack.globalToolBar.canContainHandles: true
    pageStack.globalToolBar.showNavigationButtons: applicationWindow().pageStack.currentIndex > 0 ? Kirigami.ApplicationHeaderStyle.ShowBackButton : 0

    KConfig.WindowStateSaver {
        configGroupName: "MainWindow"
    }

    Component {
        id: deviceComp
        DevicePage {}
    }

    globalDrawer: Kirigami.OverlayDrawer {
        id: drawer
        edge: Qt.application.layoutDirection === Qt.RightToLeft ? Qt.RightEdge : Qt.LeftEdge
        modal: Kirigami.Settings.isMobile || (applicationWindow().width < Kirigami.Units.gridUnit * 50 && !collapsed) // Only modal when not collapsed, otherwise collapsed won't show.
        width: Kirigami.Units.gridUnit * 16
        onModalChanged: drawerOpen = !modal

        Behavior on width {
            NumberAnimation {
                duration: Kirigami.Units.longDuration
                easing.type: Easing.InOutQuad
            }
        }
        Kirigami.Theme.colorSet: Kirigami.Theme.Window

        handleClosedIcon.source: modal ? null : "sidebar-expand-left"
        handleOpenIcon.source: modal ? null : "sidebar-collapse-left"
        handleVisible: modal

        leftPadding: 0
        rightPadding: 0
        topPadding: 0
        bottomPadding: 0

        contentItem: ColumnLayout {
            spacing: 0
            QQC2.ToolBar {
                Layout.fillWidth: true
                Layout.preferredHeight: pageStack.globalToolBar.preferredHeight

                contentItem: RowLayout {
                    spacing: 0

                    Kirigami.Heading {
                        text: i18nd("kdeconnect-app", "Devices")
                        elide: Qt.ElideRight

                        Layout.fillWidth: true
                        Layout.leftMargin: Kirigami.Units.largeSpacing
                    }

                    QQC2.ToolButton {
                        text: i18nc("@action:button", "Refresh")
                        icon.name: 'view-refresh-symbolic'

                        QQC2.ToolTip.text: text
                        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                        QQC2.ToolTip.visible: hovered

                        onClicked: {
                            //refresh
                            DaemonDbusInterface.forceOnNetworkChange();
                        }
                    }
                }
            }
            QQC2.ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                ListView {
                    id: devices
                    Layout.fillWidth: true
                    clip: true

                    section {
                        property: "status"
                        delegate: Kirigami.ListSectionHeader {
                            width: ListView.view.width
                            text: switch (parseInt(section)) {
                            case DevicesModel.Paired:
                                return i18nd("kdeconnect-app", "Remembered");
                            case DevicesModel.Reachable:
                                return i18nd("kdeconnect-app", "Available");
                            case (DevicesModel.Reachable | DevicesModel.Paired):
                                return i18nd("kdeconnect-app", "Connected");
                            }
                        }
                    }
                    Kirigami.PlaceholderMessage {
                        text: i18nd("kdeconnect-app", "No devices found")
                        icon.name: 'edit-none-symbolic'
                        anchors.centerIn: parent
                        width: parent.width - (Kirigami.Units.largeSpacing * 4)
                        visible: devices.count === 0
                    }
                    model: DevicesSortProxyModel {
                        sourceModel: DevicesModel {}
                    }
                    delegate: QQC2.ItemDelegate {
                        id: delegate
                        icon.name: iconName
                        text: model.name
                        width: ListView.view.width
                        highlighted: false

                        contentItem: Kirigami.IconTitleSubtitle {
                            title: delegate.text
                            subtitle: toolTip
                            icon: icon.fromControlsIcon(delegate.icon)
                        }

                        onClicked: {
                            pageStack.clear();
                            pageStack.push(deviceComp, {
                                currentDevice: device
                            });
                        }
                    }
                }
            }

            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
            }

            QQC2.ToolBar {
                Layout.fillWidth: true
                Layout.preferredHeight: pageStack.globalToolBar.preferredHeight
                position: QQC2.ToolBar.Footer

                contentItem: RowLayout {
                    spacing: Kirigami.Units.smallSpacing
                    QQC2.Label {
                        text: announcedNameProperty.value
                        elide: Qt.ElideRight

                        Layout.fillWidth: true
                        Layout.leftMargin: Kirigami.Units.largeSpacing

                        DBusProperty {
                            id: announcedNameProperty
                            object: DaemonDbusInterface
                            read: "announcedName"
                            defaultValue: "DeviceName"
                        }
                    }

                    QQC2.ToolButton {
                        text: i18n("Settings")
                        icon.name: "settings-configure"
                        display: QQC2.AbstractButton.IconOnly

                        QQC2.ToolTip.text: text
                        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                        QQC2.ToolTip.visible: hovered

                        onClicked: pageStack.pushDialogLayer(Qt.resolvedUrl("Settings.qml"), {}, {
                            title: i18n("Settings")
                        })
                    }
                }
            }
        }
    }

    contextDrawer: Kirigami.ContextDrawer {
        id: contextDrawer
    }

    pageStack.initialPage: Qt.resolvedUrl("WelcomePage.qml")
}
