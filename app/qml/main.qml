/*
 * SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.2
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.0 as Kirigami
import org.kde.kdeconnect 1.0

Kirigami.ApplicationWindow
{
    id: root
    visible: true
    width: 900
    height: 500

    Kirigami.Action {
        id: findDevicesAction
        text: i18nd("kdeconnect-app", "Find devices...")
        iconName: "list-add"
        checked: pageStack.currentItem && pageStack.currentItem.objectName == "FindDevices"

        onTriggered: {
            root.pageStack.clear()
            root.pageStack.push(Qt.resolvedUrl("FindDevicesPage.qml"));
        }
    }

    globalDrawer: Kirigami.GlobalDrawer {
        id: drawer

        modal: !root.wideScreen
        handleVisible: !root.wideScreen

        header: Kirigami.AbstractApplicationHeader {
            topPadding: Kirigami.Units.smallSpacing
            bottomPadding: Kirigami.Units.smallSpacing
            leftPadding: Kirigami.Units.smallSpacing
            rightPadding: Kirigami.Units.smallSpacing
            contentItem: RowLayout {
                anchors.fill: parent
                spacing: Kirigami.Units.smallSpacing

                DBusProperty {
                    id: announcedNameProperty
                    object: DaemonDbusInterface
                    read: "announcedName"
                    defaultValue: ""
                }

                TextField {
                    id: nameField
                    visible: false
                    Layout.fillWidth: true
                    text: announcedNameProperty.value
                    onAccepted: {
                        DaemonDbusInterface.setAnnouncedName(text)
                        text = Qt.binding(function() {return announcedNameProperty.value})
                    }
                }

                Kirigami.Heading {
                    level: 1
                    text: announcedNameProperty.value
                    Layout.fillWidth: true
                    visible: !nameField.visible
                    elide: Qt.ElideRight
                }

                Button {
                    icon.name: nameField.visible ? "dialog-ok-apply" : "entry-edit"
                    onClicked: {
                        nameField.visible = !nameField.visible
                        nameField.accepted()
                    }
                }
            }
        }


        topContent: RowLayout {
            width: parent.width
        }

        property var objects: [findDevicesAction]
        Instantiator {
            model: DevicesSortProxyModel {
                sourceModel: DevicesModel { displayFilter: DevicesModel.Paired | DevicesModel.Reachable }
            }
            delegate: Kirigami.Action {
                icon.name: model.iconName
                icon.color: "transparent"
                text: display + "\n" + toolTip
                visible: status & DevicesModel.Reachable
                checked: pageStack.currentItem && pageStack.currentItem.currentDevice == device
                onTriggered: {
                    root.pageStack.clear()
                    root.pageStack.push(
                        Qt.resolvedUrl("DevicePage.qml"),
                        {currentDevice: device}
                    );
                }
            }

            onObjectAdded: {
                drawer.objects.push(object)
                drawer.objects = drawer.objects
            }
            onObjectRemoved: {
                var idx = drawer.objects.indexOf(object);
                if (idx>=0) {
                    var removed = drawer.objects.splice(idx, 1)
                    drawer.objects = drawer.objects
                }
            }
        }
        actions: objects
    }

    contextDrawer: Kirigami.ContextDrawer {
        id: contextDrawer
    }

    pageStack.initialPage: Qt.resolvedUrl("FindDevicesPage.qml")
}
