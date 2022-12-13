// SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.20 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm
import org.kde.kdeconnect 1.0
import org.kde.kdeconnect.app 1.0

Kirigami.ScrollablePage {
    title: i18nc("@title:window", "Settings")

    leftPadding: 0
    rightPadding: 0

    ColumnLayout {
        MobileForm.FormCard {
            Layout.topMargin: Kirigami.Units.largeSpacing
            Layout.fillWidth: true
            contentItem: ColumnLayout {
                spacing: 0

                DBusProperty {
                    id: announcedNameProperty
                    object: DaemonDbusInterface
                    read: "announcedName"
                    defaultValue: ""
                }

                MobileForm.FormTextFieldDelegate {
                    text: announcedNameProperty.value
                    onAccepted: DaemonDbusInterface.setAnnouncedName(text);
                    label: i18n("Device name")
                }
            }
        }

        MobileForm.FormCard {
            Layout.topMargin: Kirigami.Units.largeSpacing
            Layout.fillWidth: true
            contentItem: ColumnLayout {
                spacing: 0
                Component {
                    id: aboutPage
                    MobileForm.AboutPage {
                        aboutData: About
                    }
                }
                MobileForm.FormButtonDelegate {
                    text: i18n("About KDE Connect")
                    onClicked: applicationWindow().pageStack.layers.push(aboutPage)
                }
            }
        }
    }
}
