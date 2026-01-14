/**
 * SPDX-FileCopyrightText: 2024 David Redondo <kde@david-redondo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQml.Models
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kcmutils as KCMUtils
import org.kde.kdeconnect 1.0
import org.kde.kitemmodels as KItemModels
import org.kde.kirigami as Kirigami

Kirigami.FormLayout {
    id: root

    property string device
    readonly property string pluginId: "kdeconnect_shareinputdevices"

    KdeConnectPluginConfig {
        id: config
        deviceId: device
        pluginName: pluginId

        onConfigChanged: edgeComboBox.currentIndex = edgeComboBox.indexOfValue(getInt("edge", Qt.LeftEdge))
    }

    RowLayout {
        Kirigami.FormData.label: i18nc("@label:listbox Forms a sentence: 'Leave the screen at the top/bottom/left/right edge', the pointer will switch to the connected device there", "Leave Screen at")
        QQC2.ComboBox {
            id: edgeComboBox
            textRole: "text"
            valueRole: "value"
            model: [
                {
                    text: i18nc("@item:inlistbox top edge of screen", "Top"),
                    value: Qt.TopEdge
                },
                {
                    text: i18nc("@item:inlistbox left edge of screen", "Left"),
                    value: Qt.LeftEdge
                },
                {
                    text: i18nc("@item:inlistbox right edge of screen", "Right"),
                    value: Qt.RightEdge
                },
                {
                    text: i18nc("@item:inlistbox top edge of screen", "Bottom"),
                    value: Qt.BottomEdge
                }
            ]
            Component.onCompleted: currentIndex = edgeComboBox.indexOfValue(config.getInt("edge", Qt.LeftEdge))
        }
        KCMUtils.ContextualHelpButton {
            icon.name: "data-warning"
            toolTipText: i18nc("@info:tooltip edge: screen edge", "Another device already reserved this edge. This may lead to unexpected results when both are connected at the same time.")
            visible: configInstantiator.children.some((deviceConfig) => deviceConfig.getInt("edge", Qt.LeftEdge) == edgeComboBox.currentValue)
            Instantiator {
                id: configInstantiator
                readonly property var children: Array.from({length: configInstantiator.count}, (value, index) => configInstantiator.objectAt(index))
                delegate:  KdeConnectPluginConfig {
                    deviceId: model.deviceId
                    pluginName: pluginId

                }
                model: KItemModels.KSortFilterProxyModel {
                    sourceModel: DevicesModel {
                    }
                    filterRowCallback: function(source_row, source_parent) {
                        const device = sourceModel.getDevice(source_row)
                        return  device.id() !== root.device && device.isPluginEnabled(pluginId);
                    }
                }
            }
        }
    }
}
