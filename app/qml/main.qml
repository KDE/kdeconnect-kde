/*
 * Copyright 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
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

        topContent: RowLayout {
            width: parent.width
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
                Layout.leftMargin: Kirigami.Units.smallSpacing
                text: announcedNameProperty.value
                onAccepted: {
                    DaemonDbusInterface.setAnnouncedName(text)
                    text = Qt.binding(function() {return announcedNameProperty.value})
                }
            }

            Label {
                text: announcedNameProperty.value
                visible: !nameField.visible
                Layout.fillWidth: true
                elide: Qt.ElideRight
                font.pointSize: 18
                Layout.leftMargin: Kirigami.Units.smallSpacing
            }

            Button {
                icon.name: nameField.visible ? "dialog-ok-apply" : "entry-edit"
                onClicked: {
                    nameField.visible = !nameField.visible
                    nameField.accepted()
                }
            }
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
