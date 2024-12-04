/*
 * SPDX-FileCopyrightText: 2024 Darshan Phaldesai <dev.darshanphaldesai@gmail.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */
import QtQuick
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: root
    // TODO:  replace with proper text components instead of HTML
    Kirigami.PlaceholderMessage {
        text: i18nd("kdeconnect_app", "<html><head/><body><p>No device selected<br><br>If you own an Android device, make sure to install the <a href=\"https://play.google.com/store/apps/details?id=org.kde.kdeconnect_tp\"><span style=\" text-decoration: underline;\">KDE Connect Android app</span></a> (also available <a href=\"https://f-droid.org/repository/browse/?fdid=org.kde.kdeconnect_tp\"><span style=\" text-decoration: underline;\">from F-Droid</span></a>) and it should appear in the list. If you have an iPhone, make sure to install the <a href=\"https://apps.apple.com/us/app/kde-connect/id1580245991\"><span style=\" text-decoration: underline;\">KDE Connect iOS app.</span></a> <br><br>If you are having problems, visit the <a href=\"https://userbase.kde.org/KDEConnect\"><span style=\" text-decoration: underline;\">KDE Connect Community wiki</span></a> for help.</p></body></html>")
        anchors.centerIn: parent
        width: parent.width - (Kirigami.Units.largeSpacing * 4)
    }
}
