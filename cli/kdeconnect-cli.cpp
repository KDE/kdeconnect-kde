/*
 * Copyright 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
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
#include <QCoreApplication>
#include <QTextStream>

#include <KAboutData>
#include <KLocalizedString>

#include "interfaces/devicesmodel.h"
#include "interfaces/notificationsmodel.h"
#include "interfaces/dbusinterfaces.h"
#include "kdeconnect-version.h"

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    KAboutData about("kdeconnect-cli",
                     "kdeconnect-cli",
                     QLatin1String(KDECONNECT_VERSION_STRING),
                     i18n("KDE Connect CLI tool"),
                     KAboutLicense::GPL, 
                     i18n("(C) 2015 Aleix Pol Gonzalez"));
    KAboutData::setApplicationData(about);

    about.addAuthor( i18n("Aleix Pol Gonzalez"), QString(), "aleixpol@kde.org" );
    about.addAuthor( i18n("Albert Vaca Cintora"), QString(), "albertvaka@gmail.com" );
    QCommandLineParser parser;
    parser.addOption(QCommandLineOption(QStringList("l") << "list-devices", i18n("List all devices")));
    parser.addOption(QCommandLineOption(QStringList("a") << "list-available", i18n("List available (paired and reachable) devices")));
    parser.addOption(QCommandLineOption("id-only", i18n("Make --list-devices or --list-available print only the devices id, to ease scripting")));
    parser.addOption(QCommandLineOption("refresh", i18n("Search for devices in the network and re-establish connections")));
    parser.addOption(QCommandLineOption("pair", i18n("Request pairing to a said device")));
    parser.addOption(QCommandLineOption("ring", i18n("Find the said device by ringing it.")));
    parser.addOption(QCommandLineOption("unpair", i18n("Stop pairing to a said device")));
    parser.addOption(QCommandLineOption("ping", i18n("Sends a ping to said device")));
    parser.addOption(QCommandLineOption("ping-msg", i18n("Same as ping but you can set the message to display"), i18n("message")));
    parser.addOption(QCommandLineOption("share", i18n("Share a file to a said device"), "path"));
    parser.addOption(QCommandLineOption("list-notifications", i18n("Display the notifications on a said device")));
    parser.addOption(QCommandLineOption("lock", i18n("Lock the specified device")));
    parser.addOption(QCommandLineOption(QStringList("device") << "d", i18n("Device ID"), "dev"));
    parser.addOption(QCommandLineOption("encryption-info", i18n("Get encryption info about said device")));
    about.setupCommandLine(&parser);

    parser.addHelpOption();
    parser.process(app);
    about.processCommandLine(&parser);

    const QString id = "kdeconnect-cli-"+QString::number(QCoreApplication::applicationPid());
    DaemonDbusInterface iface;

    if(parser.isSet("l") || parser.isSet("a")) {
        bool paired = true, reachable = false;
        if (parser.isSet("a")) {
            reachable = true;
        } else {
            iface.acquireDiscoveryMode(id);
            QThread::sleep(2);
        }
        QDBusPendingReply<QStringList> reply = iface.devices(paired, reachable);
        reply.waitForFinished();

        const QStringList devices = reply.value();
        foreach (const QString& id, devices) {
            if (parser.isSet("id-only")) {
                QTextStream(stdout) << id << endl;
            } else {
                DeviceDbusInterface deviceIface(id);
                QString statusInfo;
                const bool isReachable = deviceIface.isReachable(), isPaired = deviceIface.property("isPaired").toBool();
                if (isReachable && isPaired) {
                    statusInfo = i18n("(paired and reachable)");
                } else if (isReachable) {
                    statusInfo = i18n("(reachable)");
                } else if (isPaired)
                    statusInfo = i18n("(paired)");
                QTextStream(stdout) << "- " << deviceIface.name()
                        << ": " << deviceIface.id() << ' ' << statusInfo << endl;
            }
        }
        if (!parser.isSet("id-only")) {
            QTextStream(stdout) << i18np("1 device found", "%1 devices found", devices.size()) << endl;
        } else if (devices.isEmpty()) {
            QTextStream(stderr) << i18n("No devices found") << endl;
        }

        iface.releaseDiscoveryMode(id);
    } else if(parser.isSet("refresh")) {
        QDBusMessage msg = QDBusMessage::createMethodCall("org.kde.kdeconnect", "/modules/kdeconnect", "org.kde.kdeconnect.daemon", "forceOnNetworkChange");
        QDBusConnection::sessionBus().call(msg);
    } else {
        QString device;
        if(!parser.isSet("device")) {
            QTextStream(stderr) << i18n("No device specified") << endl;
        }
        device = parser.value("device");
        if(parser.isSet("share")) {
            QUrl url = QUrl::fromUserInput(parser.value("share"), QDir::currentPath());
            parser.clearPositionalArguments();
            if(!url.isEmpty() && !device.isEmpty()) {
                QDBusMessage msg = QDBusMessage::createMethodCall("org.kde.kdeconnect", "/modules/kdeconnect/devices/"+device+"/share", "org.kde.kdeconnect.device.share", "shareUrl");
                msg.setArguments(QVariantList() << url.toString());
                QDBusConnection::sessionBus().call(msg);
            } else {
                QTextStream(stderr) << (i18n("Couldn't share %1", url.toString())) << endl;
            }
        } else if(parser.isSet("pair")) {
            DeviceDbusInterface dev(device);
            if (!dev.isReachable()) {
                //Device doesn't exist, go into discovery mode and wait up to 30 seconds for the device to appear
                QEventLoop wait;
                QTextStream(stderr) << i18n("waiting for device...") << endl;
                iface.acquireDiscoveryMode(id);

                QObject::connect(&iface, &DaemonDbusInterface::deviceAdded, [&](const QString &deviceAddedId) {
                    if (device == deviceAddedId) {
                        wait.quit();
                    }
                });
                QTimer::singleShot(30 * 1000, &wait, &QEventLoop::quit);

                wait.exec();
            }

            if (!dev.isReachable()) {
                QTextStream(stderr) << i18n("Device not found") << endl;
            } else if(dev.property("isPaired").toBool()) {
                QTextStream(stderr) << i18n("Already paired") << endl;
            } else {
                QTextStream(stderr) << i18n("Pair requested") << endl;
                QDBusPendingReply<void> req = dev.requestPair();
                req.waitForFinished();
            }
            iface.releaseDiscoveryMode(id);
        } else if(parser.isSet("unpair")) {
            DeviceDbusInterface dev(device);
            if (!dev.isReachable()) {
                QTextStream(stderr) << i18n("Device does not exist") << endl;
            } else if(!dev.property("isPaired").toBool()) {
                QTextStream(stderr) << i18n("Already not paired") << endl;
            } else {
                QTextStream(stderr) << i18n("Unpaired") << endl;
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
        } else if(parser.isSet("ring")) {
            QDBusMessage msg = QDBusMessage::createMethodCall("org.kde.kdeconnect", "/modules/kdeconnect/devices/"+device+"/findmyphone", "org.kde.kdeconnect.device.findmyphone", "ring");
            QDBusConnection::sessionBus().call(msg);
        } else if(parser.isSet("list-notifications")) {
            NotificationsModel notifications;
            notifications.setDeviceId(device);
            for(int i=0, rows=notifications.rowCount(); i<rows; ++i) {
                QModelIndex idx = notifications.index(i);
                QTextStream(stdout) << "- " << idx.data(NotificationsModel::AppNameModelRole).toString()
                    << ": " << idx.data(NotificationsModel::NameModelRole).toString() << endl;
            }
        } else if(parser.isSet("encryption-info")) {
            DeviceDbusInterface dev(device);
            QDBusPendingReply<QByteArray> devReply = dev.certificate(1); // QSsl::Der = 1
            devReply.waitForFinished();
            if (devReply.value().isEmpty()) {
                QTextStream(stderr) << i18n("The other device doesn\'t use a recent version of KDE Connect, using the legacy encryption method.") << endl;
            } else {
                QByteArray remoteCertificate = QCryptographicHash::hash(devReply.value(), QCryptographicHash::Sha1).toHex();
                for (int i=2 ; i<remoteCertificate.size() ; i+=3)
                    remoteCertificate.insert(i, ':'); // Improve readability

                DaemonDbusInterface iface;
                QDBusPendingReply<QByteArray> ifaceReply = iface.certificate(1); // QSsl::Der = 1
                ifaceReply.waitForFinished();
                QByteArray myCertificate = QCryptographicHash::hash(ifaceReply.value(), QCryptographicHash::Sha1).toHex();
                for (int i=2 ; i<myCertificate.size() ; i+=3)
                    myCertificate.insert(i, ':'); // Improve readability

                QTextStream(stderr) << i18n("SHA1 fingerprint of your device certificate is : ") << myCertificate << endl;
                QTextStream(stderr) << i18n("SHA1 fingerprint of remote device certificate is : ") << remoteCertificate << endl;
            }
        } else {
            QTextStream(stderr) << i18n("Nothing to be done") << endl;
        }
    }
    QMetaObject::invokeMethod(&app, "quit", Qt::QueuedConnection);

    return app.exec();
}
