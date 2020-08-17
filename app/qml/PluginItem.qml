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

QtObject
{
    property alias pluginName: checker.pluginName
    property alias iconName: checker.iconName
    property alias loaded: checker.available
    property alias device: checker.device
    property var interfaceFactory
    property var component
    property var name

    readonly property var checker: PluginChecker {
        id: checker
    }
    property var onClick: () => {
        if (component === "" || !interfaceFactory)
            return;

        var obj = interfaceFactory.create(checker.device.id());
        var page = pageStack.push(
            component,
            { pluginInterface: obj,
              device: checker.device
            }
        );
        obj.parent = page
    }
}
