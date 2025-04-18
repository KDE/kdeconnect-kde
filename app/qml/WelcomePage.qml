/*
 * SPDX-FileCopyrightText: 2024 Darshan Phaldesai <dev.darshanphaldesai@gmail.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCardPage {
    id: root


    Kirigami.Icon {
        source: "kdeconnect"
        implicitWidth: Kirigami.Units.iconSizes.huge
        implicitHeight: Kirigami.Units.iconSizes.huge

        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: Kirigami.Units.gridUnit
    }

    Kirigami.Heading {
        text: i18nc("@title", "No Device Selected")
        horizontalAlignment: Qt.AlignHCenter

        Layout.fillWidth: true
        Layout.topMargin: Kirigami.Units.largeSpacing
    }

    Controls.Label {
        text: i18nc("@info", "Make sure to install KDE Connect on your other devices.")
        horizontalAlignment: Qt.AlignHCenter
        wrapMode: Text.WordWrap

        Layout.fillWidth: true
    }

    FormCard.FormHeader {
        title: i18nc("@title:context", "KDE Connect Android App")

        Layout.topMargin: Kirigami.Units.gridUnit
    }

    FormCard.FormCard {
        FormCard.FormButtonDelegate {
            text: i18nc("Android Application Store", "F-Droid")
            onClicked: Qt.openUrlExternally("https://f-droid.org/repository/browse/?fdid=org.kde.kdeconnect_tp")
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormButtonDelegate {
            text: i18nc("Android Application Store", "Play Store")
            onClicked: Qt.openUrlExternally("https://play.google.com/store/apps/details?id=org.kde.kdeconnect_tp")
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title:context", "KDE Connect iOS App")
    }

    FormCard.FormCard {
        FormCard.FormButtonDelegate {
            text: i18n("Apple Store")
            onClicked: Qt.openUrlExternally("https://apps.apple.com/us/app/kde-connect/id1580245991")
        }
    }

    FormCard.FormSectionText {
        text: i18n("If you are having problems, visit the <a href=\"https://userbase.kde.org/KDEConnect\"><span style=\" text-decoration: underline;\">KDE Connect Community wiki</span></a> for help.")
    }
}
