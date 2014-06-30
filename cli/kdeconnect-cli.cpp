/*
 * Copyright 2013 Aleix Pol Gonzalez <aleixpol@kde.org>
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

#include <KApplication>
#include <KUrl>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <interfaces/devicesmodel.h>
#include <interfaces/notificationsmodel.h>
#include <interfaces/dbusinterfaces.h>
#include <iostream>
#include <QDBusMessage>
#include <QDBusConnection>

int main(int argc, char** argv)
{
    KAboutData about("kdeconnect-cli", "kdeconnect-cli", ki18n(("kdeconnect-cli")), "1.0", ki18n("KDE Connect CLI tool"),
                     KAboutData::License_GPL, ki18n("(C) 2013 Aleix Pol Gonzalez"));
    about.addAuthor( ki18n("Aleix Pol Gonzalez"), KLocalizedString(), "aleixpol@kde.org" );
    KCmdLineArgs::init(argc, argv, &about);
    KCmdLineOptions options;
    options.add("l")
           .add("list-devices", ki18n("List all devices"));
    options.add("share <path>", ki18n("Share a file to a said device"));
    options.add("pair", ki18n("Request pairing to a said device"));
    options.add("unpair", ki18n("Stop pairing to a said device"));
    options.add("ping", ki18n("Sends a ping to said device"));
    options.add("list-notifications", ki18n("Display the notifications on a said device"));
    options.add("device <dev>", ki18n("Device ID"));
    KCmdLineArgs::addCmdLineOptions( options );
    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
    KApplication app;
    if(args->isSet("l")) {
        DevicesModel devices;
        for(int i=0, rows=devices.rowCount(); i<rows; ++i) {
            QModelIndex idx = devices.index(i);
            QString statusInfo;
            switch(idx.data(DevicesModel::StatusModelRole).toInt()) {
                case DevicesModel::StatusPaired:
                    statusInfo = "(paired)";
                    break;
                case DevicesModel::StatusReachable:
                    statusInfo = "(reachable)";
                    break;
                case DevicesModel::StatusReachable | DevicesModel::StatusPaired:
                    statusInfo = "(paired and reachable)";
                    break;
            }
            std::cout << "- " << idx.data(Qt::DisplayRole).toString().toStdString()
                      << ": " << idx.data(DevicesModel::IdModelRole).toString().toStdString() << ' ' << statusInfo.toStdString() << std::endl;
        }
        std::cout << devices.rowCount() << " devices found" << std::endl;
    } else {
        QString device;
        if(!args->isSet("device")) {
            KCmdLineArgs::usageError(i18n("No device specified"));
        }
        device = args->getOption("device");
        QUrl url;
        if(args->isSet("share")) {
            url = args->makeURL(args->getOption("share").toLatin1());
            args->clear();
            if(!url.isEmpty() && !device.isEmpty()) {
                QDBusMessage msg = QDBusMessage::createMethodCall("org.kde.kdeconnect", "/modules/kdeconnect/devices/"+device+"/share", "org.kde.kdeconnect.device.share", "shareUrl");
                msg.setArguments(QVariantList() << url.toString());
                QDBusConnection::sessionBus().call(msg);
            } else
                KCmdLineArgs::usageError(i18n("Couldn't share %1", url.toString()));
        } else if(args->isSet("pair")) {
            DeviceDbusInterface dev(device);
            if(dev.isPaired())
                std::cout << "Already paired" << std::endl;
            else {
                QDBusPendingReply<void> req = dev.requestPair();
                req.waitForFinished();
            }
        } else if(args->isSet("unpair")) {
            DeviceDbusInterface dev(device);
            if(!dev.isPaired())
                std::cout << "Already not paired" << std::endl;
            else {
                QDBusPendingReply<void> req = dev.unpair();
                req.waitForFinished();
            }
        } else if(args->isSet("ping")) {
            QDBusMessage msg = QDBusMessage::createMethodCall("org.kde.kdeconnect", "/modules/kdeconnect/devices/"+device+"/ping", "org.kde.kdeconnect.device.ping", "sendPing");
            QDBusConnection::sessionBus().call(msg);
        } else if(args->isSet("list-notifications")) {
            NotificationsModel notifications;
            notifications.setDeviceId(device);
            for(int i=0, rows=notifications.rowCount(); i<rows; ++i) {
                QModelIndex idx = notifications.index(i);
                std::cout << "- " << idx.data(NotificationsModel::AppNameModelRole).toString().toStdString()
                << ": " << idx.data(NotificationsModel::NameModelRole).toString().toStdString() << std::endl;
            }
        } else {
            KCmdLineArgs::usageError(i18n("Nothing to be done with the device"));
        }
    }
    QMetaObject::invokeMethod(&app, "quit", Qt::QueuedConnection);

    return app.exec();
}
