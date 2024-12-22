// SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.kdeconnect
import org.kde.kdeconnect.app

FormCard.FormCardPage {
    title: i18nc("@title:window", "Settings")

    FormCard.FormCard {
        Layout.topMargin: Kirigami.Units.gridUnit

        FormCard.FormTextFieldDelegate {
            text: announcedNameProperty.value
            onAccepted: DaemonDbusInterface.setAnnouncedName(text);
            label: i18n("Device name")

            DBusProperty {
                id: announcedNameProperty
                object: DaemonDbusInterface
                read: "announcedName"
                defaultValue: ""
            }
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Backends")
    }

    FormCard.FormCard {
        DBusProperty {
            id: linkProvidersProperty
            object: DaemonDbusInterface
            read: "linkProviders"
            defaultValue: []
        }

        Repeater {
            model: linkProvidersProperty.value

            FormCard.FormCheckDelegate {
                required property string modelData

                readonly property string linkProviderId: modelData.split('|')[0]

                checked: modelData.split('|')[1] === 'enabled'
                text: linkProviderId

                onToggled: DaemonDbusInterface.setLinkProviderState(linkProviderId, checked);
            }
        }
    }

    FormCard.FormCard {
        Layout.topMargin: Kirigami.Units.gridUnit

        FormCard.FormButtonDelegate {
            text: i18n("About KDE Connect")
            onClicked: applicationWindow().pageStack.layers.push(Qt.createComponent("org.kde.kirigamiaddons.formcard", "AboutPage"))
            icon.name: 'kdeconnect'
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormButtonDelegate {
            text: i18n("About KDE")
            onClicked: applicationWindow().pageStack.layers.push(Qt.createComponent("org.kde.kirigamiaddons.formcard", "AboutKDEPage"))
            icon.name: 'kde'
        }
    }
}
