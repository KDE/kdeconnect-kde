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

#include <QCryptographicHash>
#include <QIODevice>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QApplication>
#include <QTextStream>
#include <QSystemTrayIcon>

#include <KAboutData>
#include <KLocalizedString>

#include "interfaces/devicesmodel.h"
#include "interfaces/notificationsmodel.h"
#include "interfaces/dbusinterfaces.h"
#include "kdeconnect-version.h"
#include "deviceindicator.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    KAboutData about("kdeconnect-cli",
                     i18n("KDE Connect Indicator"),
                     QStringLiteral(KDECONNECT_VERSION_STRING),
                     i18n("KDE Connect Indicator tool"),
                     KAboutLicense::GPL,
                     i18n("(C) 2016 Aleix Pol Gonzalez"));
    KAboutData::setApplicationData(about);

    DevicesModel model;

    QMenu menu;
    QSystemTrayIcon systray;
    systray.setIcon(QIcon::fromTheme("kdeconnect"));
    systray.setContextMenu(&menu);
    systray.setVisible(true);

    QObject::connect(&model, &DevicesModel::rowsInserted, &model, [&menu, &model](const QModelIndex& /*parent*/, int first, int last) {
        qDebug() << "wooooo" << first << last;
        for (int i=first; i<=last; ++i) {
            DeviceDbusInterface* device = model.getDevice(first);
            auto indicator = new DeviceIndicator(device);
            QObject::connect(device, &DeviceDbusInterface::destroyed, indicator, &QObject::deleteLater);

            menu.addMenu(indicator);
        }
    });

    return app.exec();
}
