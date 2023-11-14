/*
 * SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.20 as Kirigami
import org.kde.kdeconnect 1.0

Kirigami.ApplicationWindow {
    id: root
    property int columnWidth: Kirigami.Units.gridUnit * 13
    minimumWidth: Kirigami.Units.gridUnit * 15
    minimumHeight: Kirigami.Units.gridUnit * 15
    wideScreen: width > columnWidth * 5
    pageStack.globalToolBar.canContainHandles: true
    pageStack.globalToolBar.showNavigationButtons: applicationWindow().pageStack.currentIndex > 0 ? Kirigami.ApplicationHeaderStyle.ShowBackButton : 0

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

                leftPadding: Kirigami.Units.largeSpacing
                rightPadding: Kirigami.Units.largeSpacing
                topPadding: Kirigami.Units.smallSpacing
                bottomPadding: Kirigami.Units.smallSpacing

                contentItem: Kirigami.Heading {
                    text: announcedNameProperty.value
                    elide: Qt.ElideRight

                    DBusProperty {
                        id: announcedNameProperty
                        object: DaemonDbusInterface
                        read: "announcedName"
                        defaultValue: ""
                    }
                }
            }

            QQC2.ItemDelegate {
                id: findDevicesAction
                text: i18nd("kdeconnect-app", "Find devices...")
                icon.name: "list-add"
                checked: pageStack.currentItem && pageStack.currentItem.objectName == "FindDevices"
                Layout.fillWidth: true

                onClicked: {
                    root.pageStack.clear()
                    root.pageStack.push(Qt.resolvedUrl("FindDevicesPage.qml"));
                }
            }

            Kirigami.Separator {
                Layout.fillWidth: true
            }

            Repeater {
                model: DevicesSortProxyModel {
                    sourceModel: DevicesModel {
                        displayFilter: DevicesModel.Paired | DevicesModel.Reachable
                    }
                }

                QQC2.ItemDelegate {
                    Layout.fillWidth: true
                    text: model.name + "\n" + toolTip
                    enabled: status & DevicesModel.Reachable
                    checked: pageStack.currentItem && pageStack.currentItem.currentDevice == device
                    icon.name: model.iconName
                    icon.color: "transparent"
                    onClicked: {
                        root.pageStack.clear()
                        root.pageStack.push(
                            Qt.resolvedUrl("DevicePage.qml"),
                            {currentDevice: device}
                        );
                    }
                }
            }

            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
            }

            QQC2.ItemDelegate {
                text: i18n("Settings")
                icon.name: "settings-configure"
                Layout.fillWidth: true
                onClicked: pageStack.pushDialogLayer('qrc:/qml/Settings.qml', {}, {
                    title: i18n("Settings"),
                });
            }
        }
    }

    contextDrawer: Kirigami.ContextDrawer {
        id: contextDrawer
    }

    pageStack.initialPage: Qt.resolvedUrl("FindDevicesPage.qml")
}
