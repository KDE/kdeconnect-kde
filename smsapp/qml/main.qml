/**
 * SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2018 Nicolas Fella <nicolas.fella@gmx.de>
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.1
import org.kde.kirigami 2.6 as Kirigami
import org.kde.kdeconnect 1.0

Kirigami.ApplicationWindow
{
    id: root
    visible: true
    width: 800
    height: 600

    pageStack.initialPage: ConversationList {
        title: i18nd("kdeconnect-sms", "KDE Connect SMS")
        initialMessage: initialMessage
    }

    Component {
        id: aboutPageComponent
        Kirigami.AboutPage {}
    }

    globalDrawer: Kirigami.GlobalDrawer {

        isMenu: true

        actions: [
            Kirigami.Action {
                text: i18nd("kdeconnect-sms", "About")
                icon.name: "help-about"
                onTriggered: {
                    if (applicationWindow().pageStack.layers.depth < 2) {
                        applicationWindow().pageStack.layers.push(aboutPageComponent, { aboutData: aboutData })
                    }
                }
            }
        ]
    }

}
