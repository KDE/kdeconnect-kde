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
import org.kde.kirigami 1.0 as Kirigami
import org.kde.kdeconnect 1.0

Kirigami.ApplicationWindow
{
    id: root
    visible: true
    width: 400
    height: 500

    globalDrawer: Kirigami.GlobalDrawer {
        title: i18n("KDE Connect")
        titleIcon: "kdeconnect"
//         bannerImageSource: "/home/apol/devel/kde5/share/wallpapers/Next/contents/images/1024x768.png"

        content: ListView {
            anchors.fill: parent
            model: DevicesSortProxyModel {
                sourceModel: DevicesModel { displayFilter: DevicesModel.Paired }
            }
            header: Kirigami.BasicListItem {
                label: i18n ("Find devices...")
                icon: "list-add"
                onClicked: {
                    root.pageStack.clear()
                    root.pageStack.push("qrc:/qml/FindDevicesPage.qml");
                }
            }

            delegate: Kirigami.BasicListItem {
                width: ListView.view.width
                icon: iconName
                label: display + "\n" + toolTip
                enabled: status & DevicesModel.Reachable
                checked: root.pageStack.currentDevice == device
                onClicked: {
                    root.pageStack.clear()
                    root.pageStack.push(
                        "qrc:/qml/DevicePage.qml",
                        {currentDevice: device}
                    );
                }
            }
        }
    }

    contextDrawer: Kirigami.ContextDrawer {
        id: contextDrawer
    }

    pageStack.initialPage: FindDevicesPage {}
}
