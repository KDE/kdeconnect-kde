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

#include <interfaces/devicesmodel.h>
#include <interfaces/notificationsmodel.h>
#include <interfaces/dbusinterfaces.h>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QGuiApplication>
#include <KAboutData>
#include <KLocalizedString>
#include <QTextStream>

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    KAboutData about("kdeconnect-cli", i18n("kdeconnect-cli"), "1.0", i18n("KDE Connect CLI tool"),
                     KAboutLicense::GPL, i18n("(C) 2013 Aleix Pol Gonzalez"));
    KAboutData::setApplicationData(about);

    about.addAuthor( i18n("Aleix Pol Gonzalez"), QString(), "aleixpol@kde.org" );
    about.addAuthor( i18n("Albert Vaca Cintora"), QString(), "albertvaka@gmail.com" );
    QCommandLineParser parser;
    parser.addOption(QCommandLineOption(QStringList("l") << "list-devices", i18n("List all devices")));
    parser.addOption(QCommandLineOption("refresh", i18n("Search for devices in the network and re-establishe connections")));
    parser.addOption(QCommandLineOption("pair", i18n("Request pairing to a said device")));
    parser.addOption(QCommandLineOption("unpair", i18n("Stop pairing to a said device")));
    parser.addOption(QCommandLineOption("ping", i18n("Sends a ping to said device")));
    parser.addOption(QCommandLineOption("ping-msg", i18n("Same as ping but you can set the message to display"), i18n("message")));
    parser.addOption(QCommandLineOption("share", i18n("Share a file to a said device"), "path"));
    parser.addOption(QCommandLineOption("list-notifications", i18n("Display the notifications on a said device")));
    parser.addOption(QCommandLineOption(QStringList("device") << "d", i18n("Device ID"), "dev"));
    about.setupCommandLine(&parser);

    parser.addHelpOption();
    parser.process(app);
    about.processCommandLine(&parser);

    if(parser.isSet("l")) {
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
            QTextStream(stdout) << "- " << idx.data(Qt::DisplayRole).toString()
                      << ": " << idx.data(DevicesModel::IdModelRole).toString() << ' ' << statusInfo << endl;
        }
        QTextStream(stdout) << devices.rowCount() << " devices found" << endl;
    } else if(parser.isSet("refresh")) {
        QDBusMessage msg = QDBusMessage::createMethodCall("org.kde.kdeconnect", "/modules/kdeconnect", "org.kde.kdeconnect.daemon", "forceOnNetworkChange");
        QDBusConnection::sessionBus().call(msg);
    } else {
        QString device;
        if(!parser.isSet("device")) {
            qCritical() << (i18n("No device specified"));
        }
        device = parser.value("device");
        QUrl url;
        if(parser.isSet("share")) {
            url = QUrl::fromUserInput(parser.value("share"));
            parser.clearPositionalArguments();
            if(!url.isEmpty() && !device.isEmpty()) {
                QDBusMessage msg = QDBusMessage::createMethodCall("org.kde.kdeconnect", "/modules/kdeconnect/devices/"+device+"/share", "org.kde.kdeconnect.device.share", "shareUrl");
                msg.setArguments(QVariantList() << url.toString());
                QDBusConnection::sessionBus().call(msg);
            } else
                qCritical() << (i18n("Couldn't share %1", url.toString()));
        } else if(parser.isSet("pair")) {
            DeviceDbusInterface dev(device);
            if(dev.isPaired())
                QTextStream(stdout) << "Already paired" << endl;
            else {
                QDBusPendingReply<void> req = dev.requestPair();
                req.waitForFinished();
            }
        } else if(parser.isSet("unpair")) {
            DeviceDbusInterface dev(device);
            if(!dev.isPaired())
                QTextStream(stdout) << "Already not paired" << endl;
            else {
                QDBusPendingReply<void> req = dev.unpair();
                req.waitForFinished();
            }
        } else if(parser.isSet("ping") || parser.isSet("ping-msg")) {
            QDBusMessage msg = QDBusMessage::createMethodCall("org.kde.kdeconnect", "/modules/kdeconnect/devices/"+device+"/ping", "org.kde.kdeconnect.device.ping", "sendPing");
            if (parser.isSet("ping-msg")) {
                QString message = parser.value("ping-msg");
                msg.setArguments(QVariantList() << message);
            }
            QDBusConnection::sessionBus().call(msg);
        } else if(parser.isSet("list-notifications")) {
            NotificationsModel notifications;
            notifications.setDeviceId(device);
            for(int i=0, rows=notifications.rowCount(); i<rows; ++i) {
                QModelIndex idx = notifications.index(i);
                QTextStream(stdout) << "- " << idx.data(NotificationsModel::AppNameModelRole).toString()
                << ": " << idx.data(NotificationsModel::NameModelRole).toString() << endl;
            }
        } else {
            qCritical() << i18n("Nothing to be done with the device");
        }
    }
    QMetaObject::invokeMethod(&app, "quit", Qt::QueuedConnection);

    return app.exec();
}
