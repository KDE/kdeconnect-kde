/*
 * Copyright 2016 Aleix Pol Gonzalez <aleixpol@kde.org>
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

Kirigami.Page
{
    objectName: "FindDevices"
    title: i18n("Pair")
    ScrollView {
        anchors.fill: parent
        ListView {
            section {
                property: "status"
                delegate: Label {
                    text: switch (parseInt(section))
                    {
                        case DevicesModel.Paired:
                            return i18n("Paired")
                        case DevicesModel.Reachable:
                            return i18n("Reachable")
                        case (DevicesModel.Reachable | DevicesModel.Paired):
                            return i18n("Paired & Reachable")
                    }

                }
            }

            model: DevicesSortProxyModel {
                sourceModel: DevicesModel { displayFilter: DevicesModel.Reachable }
            }
            delegate: Kirigami.BasicListItem {
                width: ListView.view.width
                icon: iconName
                label: display + "\n" + toolTip
                enabled: !(status & DevicesModel.Paired)
                onClicked: {
                    pageStack.clear()
                    pageStack.push(
                        "qrc:/qml/DevicePage.qml",
                        {currentDevice: device}
                    );
                }
            }
        }
    }
}
