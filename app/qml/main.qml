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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.0 as Kirigami
import org.kde.kdeconnect 1.0

Kirigami.ApplicationWindow
{
    id: root
    visible: true
    width: 900
    height: 500

    Component {
        id: findDevicesComp
        FindDevicesPage {}
    }

    Component {
        id: deviceComp
        DevicePage {}
    }

    Kirigami.Action {
        id: findDevicesAction
        text: i18n ("Find devices...")
        iconName: "list-add"
        checked: pageStack.currentItem && pageStack.currentItem.objectName == "FindDevices"

        onTriggered: {
            root.pageStack.clear()
            root.pageStack.push(findDevicesComp);
        }
    }

    globalDrawer: Kirigami.GlobalDrawer {
        id: drawer
        title: i18n("KDE Connect")
        titleIcon: "kdeconnect"
//         bannerImageSource: "/home/apol/devel/kde5/share/wallpapers/Next/contents/images/1024x768.png"

        topContent: [
            TextField {
                Layout.fillWidth: true

                DBusProperty {
                    id: announcedNameProperty
                    object: DaemonDbusInterface
                    read: "announcedName"
                    defaultValue: ""
                }

                text: announcedNameProperty.value
                onAccepted: {
                    DaemonDbusInterface.setAnnouncedName(text)
                    text = Qt.binding(function() {return announcedNameProperty.value})
                }
            }
        ]
        property var objects: [findDevicesAction]
        Instantiator {
            model: DevicesSortProxyModel {
                sourceModel: DevicesModel { displayFilter: DevicesModel.Paired }
            }
            delegate: Kirigami.Action {
                iconName: model.iconName
                text: display + "\n" + toolTip
                enabled: status & DevicesModel.Reachable
                checked: pageStack.currentItem && pageStack.currentItem.currentDevice == device
                onTriggered: {
                    root.pageStack.clear()
                    root.pageStack.push(
                        deviceComp,
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

    pageStack.initialPage: findDevicesComp
}
