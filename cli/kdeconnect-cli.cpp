/*
 * SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDBusMessage>
#include <QFile>
#include <QIODevice>
#include <QTextStream>

#include <KAboutData>
#include <KCrash>

#include "interfaces/conversationmessage.h"
#include "interfaces/dbushelpers.h"
#include "interfaces/dbusinterfaces.h"
#include "interfaces/devicesmodel.h"
#include "interfaces/notificationsmodel.h"
#include "kdeconnect-version.h"

#include <dbushelper.h>

QString getDevice(DaemonDbusInterface &iface, const QCommandLineParser &parser)
{
    QString device = parser.value(QStringLiteral("device"));
    if (device.isEmpty() && parser.isSet(QStringLiteral("name"))) {
        device = blockOnReply(iface.deviceIdByName(parser.value(QStringLiteral("name"))));
        if (device.isEmpty()) {
            QTextStream(stderr) << "Couldn't find device: " << parser.value(QStringLiteral("name")) << Qt::endl;
            return QString();
        }
    }
    if (device.isEmpty()) {
        QTextStream(stderr) << i18n(
            "No device specified: Use -d <Device ID> or -n <Device Name> to specify a device. \nDevice ID's and names may be found using \"kdeconnect-cli "
            "-l\" \nView complete help with --help option")
                            << Qt::endl;
        return QString();
    }

    if (!blockOnReply<QStringList>(iface.devices(false, false)).contains(device)) {
        QTextStream(stderr) << "Couldn't find device with id \"" << device << "\". To specify a device by name use -n <devicename>" << Qt::endl;
        return QString();
    }

    return device;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    KAboutData aboutData(QStringLiteral("kdeconnect-cli"),
                         QStringLiteral("kdeconnect-cli"),
                         QStringLiteral(KDECONNECT_VERSION_STRING),
                         i18n("KDE Connect CLI tool"),
                         KAboutLicense::GPL,
                         i18n("© 2015–2025, KDE Connect Team"));
    aboutData.addAuthor(i18n("Aleix Pol Gonzalez"), {}, QStringLiteral("aleixpol@kde.org"));
    aboutData.addAuthor(i18n("Albert Vaca Cintora"), {}, QStringLiteral("albertvaka@kde.org"));
    KAboutData::setApplicationData(aboutData);

    KCrash::initialize();

    QCommandLineParser parser;
    parser.addOption(QCommandLineOption(QStringList{QStringLiteral("l"), QStringLiteral("list-devices")}, i18n("List all devices")));
    parser.addOption(
        QCommandLineOption(QStringList{QStringLiteral("a"), QStringLiteral("list-available")}, i18n("List available (paired and reachable) devices")));
    parser.addOption(QCommandLineOption(QStringList{QStringLiteral("b"), QStringLiteral("list-backends")}, i18n("List all backends and status")));
    parser.addOption(QCommandLineOption(QStringLiteral("enable-backend"), i18n("Enable the specified backend"), i18n("backend")));
    parser.addOption(QCommandLineOption(QStringLiteral("disable-backend"), i18n("Disable the specified backend"), i18n("backend")));
    parser.addOption(
        QCommandLineOption(QStringLiteral("id-only"), i18n("Make --list-devices or --list-available print only the devices id, to ease scripting")));
    parser.addOption(
        QCommandLineOption(QStringLiteral("name-only"), i18n("Make --list-devices or --list-available print only the devices name, to ease scripting")));
    parser.addOption(QCommandLineOption(QStringLiteral("id-name-only"),
                                        i18n("Make --list-devices or --list-available print only the devices id and name, to ease scripting")));
    parser.addOption(QCommandLineOption(QStringLiteral("refresh"), i18n("Search for devices in the network and re-establish connections")));
    parser.addOption(QCommandLineOption(QStringLiteral("pair"), i18n("Request pairing with device")));
    parser.addOption(QCommandLineOption(QStringLiteral("ring"), i18n("Find device by ringing it")));
    parser.addOption(QCommandLineOption(QStringLiteral("unpair"), i18n("Unpair from device")));
    parser.addOption(QCommandLineOption(QStringLiteral("ping"), i18n("Send ping to device")));
    parser.addOption(QCommandLineOption(QStringLiteral("ping-msg"), i18n("Same as ping but you can set the message to display"), i18n("message")));
    parser.addOption(QCommandLineOption(QStringLiteral("send-clipboard"), i18n("Send clipboard to device")));
    parser.addOption(QCommandLineOption(QStringLiteral("share"), i18n("Share file/URL to device"), QStringLiteral("path or URL")));
    parser.addOption(QCommandLineOption(QStringLiteral("share-text"), i18n("Share text to device"), QStringLiteral("text")));
    parser.addOption(QCommandLineOption(QStringLiteral("list-notifications"), i18n("Display notifications on device")));
    parser.addOption(QCommandLineOption(QStringLiteral("lock"), i18n("Lock the specified device")));
    parser.addOption(QCommandLineOption(QStringLiteral("unlock"), i18n("Unlock the specified device")));
    parser.addOption(QCommandLineOption(QStringLiteral("send-sms"), i18n("Sends an SMS. Requires destination"), i18n("message")));
    parser.addOption(QCommandLineOption(QStringLiteral("destination"), i18n("Phone number to send the message"), i18n("phone number")));
    parser.addOption(QCommandLineOption(QStringLiteral("attachment"),
                                        i18n("File URLs to send attachments with the message (can be passed multiple times)"),
                                        i18n("file URLs")));
    parser.addOption(QCommandLineOption(QStringList{QStringLiteral("device"), QStringLiteral("d")}, i18n("Device ID"), QStringLiteral("dev")));
    parser.addOption(QCommandLineOption(QStringList{QStringLiteral("name"), QStringLiteral("n")}, i18n("Device Name"), QStringLiteral("name")));
    parser.addOption(QCommandLineOption(QStringLiteral("encryption-info"), i18n("Get encryption info about device")));
    parser.addOption(QCommandLineOption(QStringLiteral("list-commands"), i18n("List remote commands and their ids")));
    parser.addOption(QCommandLineOption(QStringLiteral("execute-command"), i18n("Execute remote command by id"), QStringLiteral("id")));
    parser.addOption(QCommandLineOption(QStringList{QStringLiteral("k"), QStringLiteral("send-keys")}, i18n("Send keys to device"), QStringLiteral("key")));
    parser.addOption(QCommandLineOption(QStringLiteral("my-id"), i18n("Display this device's id and exit")));
    parser.addOption(QCommandLineOption(QStringLiteral("mount"), i18n("Mount filesystem of a said device")));
    parser.addOption(QCommandLineOption(QStringLiteral("get-mount-point"), i18n("Print mount point for the filesystem of a said device")));

    // Hidden because it's an implementation detail
    QCommandLineOption deviceAutocomplete(QStringLiteral("shell-device-autocompletion"));
    deviceAutocomplete.setFlags(QCommandLineOption::HiddenFromHelp);
    deviceAutocomplete.setDescription(
        QStringLiteral("Outputs all available devices id's with their name and paired status")); // Not visible, so no translation needed
    deviceAutocomplete.setValueName(QStringLiteral("shell"));
    parser.addOption(deviceAutocomplete);
    aboutData.setupCommandLine(&parser);

    parser.process(app);
    aboutData.processCommandLine(&parser);

    DaemonDbusInterface iface;

    if (parser.isSet(QStringLiteral("my-id"))) {
        QTextStream(stdout) << iface.selfId() << Qt::endl;
    } else if (parser.isSet(QStringLiteral("l")) || parser.isSet(QStringLiteral("a"))) {
        bool available = false;
        if (parser.isSet(QStringLiteral("a"))) {
            available = true;
        } else {
            QThread::sleep(2);
        }
        const QStringList devices = blockOnReply<QStringList>(iface.devices(available, available));

        bool displayCount = true;
        for (const QString &id : devices) {
            if (parser.isSet(QStringLiteral("id-only"))) {
                QTextStream(stdout) << id << Qt::endl;
                displayCount = false;
            } else if (parser.isSet(QStringLiteral("name-only"))) {
                DeviceDbusInterface deviceIface(id);
                QTextStream(stdout) << deviceIface.name() << Qt::endl;
                displayCount = false;
            } else if (parser.isSet(QStringLiteral("id-name-only"))) {
                DeviceDbusInterface deviceIface(id);
                QTextStream(stdout) << id << ' ' << deviceIface.name() << Qt::endl;
                displayCount = false;
            } else {
                DeviceDbusInterface deviceIface(id);
                QString statusInfo;
                const bool isReachable = deviceIface.isReachable();
                const bool isPaired = deviceIface.isPaired();
                if (isReachable && isPaired) {
                    statusInfo = i18n("(paired and reachable)");
                } else if (isReachable) {
                    statusInfo = i18n("(reachable)");
                } else if (isPaired) {
                    statusInfo = i18n("(paired)");
                }

                QTextStream(stdout) << "- " << deviceIface.name() << ": " << deviceIface.id() << QStringLiteral(" on ")
                                    << deviceIface.reachableAddresses().join(QStringLiteral(", ")) << QStringLiteral(" via ")
                                    << deviceIface.activeProviderNames().join(QStringLiteral(", ")) << ' ' << statusInfo << Qt::endl;
            }
        }
        if (displayCount) {
            QTextStream(stderr) << i18np("1 device found", "%1 devices found", devices.size()) << Qt::endl;
        } else if (devices.isEmpty()) {
            QTextStream(stderr) << i18n("No devices found") << Qt::endl;
        }
    } else if (parser.isSet(QStringLiteral("b"))) {
        const QStringList backends = blockOnReply<QStringList>(iface.linkProviders());
        for (const QString &backend : backends) {
            QTextStream(stdout) << backend << Qt::endl;
        }
    } else if (parser.isSet(QStringLiteral("shell-device-autocompletion"))) {
        // Outputs a list of reachable devices in zsh autocomplete format, with the name as description
        const QStringList devices = blockOnReply<QStringList>(iface.devices(true, false));
        for (const QString &id : devices) {
            DeviceDbusInterface deviceIface(id);
            QString statusInfo;
            const bool isPaired = deviceIface.isPaired();
            if (isPaired) {
                statusInfo = i18n("(paired)");
            } else {
                statusInfo = i18n("(unpaired)");
            }

            // Description: "device name (paired/unpaired)"
            QString description = deviceIface.name() + QLatin1Char(' ') + statusInfo;
            // Replace characters
            description.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
            description.replace(QLatin1Char('['), QStringLiteral("\\["));
            description.replace(QLatin1Char(']'), QStringLiteral("\\]"));
            description.replace(QLatin1Char('\''), QStringLiteral("\\'"));
            description.replace(QLatin1Char('\"'), QStringLiteral("\\\""));
            description.replace(QLatin1Char('\n'), QLatin1Char(' '));
            description.remove(QLatin1Char('\0'));

            // Output id and description
            QTextStream(stdout) << id << '[' << description << ']' << Qt::endl;
        }

        // Exit with 1 if we didn't find a device
        return int(devices.isEmpty());
    } else if (parser.isSet(QStringLiteral("refresh"))) {
        QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kdeconnect"),
                                                          QStringLiteral("/modules/kdeconnect"),
                                                          QStringLiteral("org.kde.kdeconnect.daemon"),
                                                          QStringLiteral("forceOnNetworkChange"));
        blockOnReply(QDBusConnection::sessionBus().asyncCall(msg));
    } else if (parser.isSet(QStringLiteral("enable-backend"))) {
        const QString backendName = parser.value(QStringLiteral("enable-backend"));
        blockOnReply(iface.setLinkProviderState(backendName, true));
    } else if (parser.isSet(QStringLiteral("disable-backend"))) {
        const QString backendName = parser.value(QStringLiteral("disable-backend"));
        blockOnReply(iface.setLinkProviderState(backendName, false));
    } else if (parser.isSet(QStringLiteral("share"))) {
        QString device = getDevice(iface, parser);
        if (device.isEmpty())
            return 1;

        QStringList urls;

        QString firstArg = parser.value(QStringLiteral("share"));
        const auto args = QStringList(firstArg) + parser.positionalArguments();

        for (const QString &input : args) {
            QUrl url = QUrl::fromUserInput(input, QDir::currentPath());
            if (url.isEmpty()) {
                qWarning() << "URL not valid:" << input;
                continue;
            }
            urls.append(url.toString());
        }

        QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kdeconnect"),
                                                          QLatin1String("/modules/kdeconnect/devices/%1/share").arg(device),
                                                          QStringLiteral("org.kde.kdeconnect.device.share"),
                                                          QStringLiteral("shareUrls"));

        msg.setArguments(QVariantList{QVariant(urls)});
        blockOnReply(QDBusConnection::sessionBus().asyncCall(msg));

        for (const QString &url : std::as_const(urls)) {
            QTextStream(stdout) << i18nc("A file or URL was shared", "Shared %1", url) << Qt::endl;
        }
    } else if (parser.isSet(QStringLiteral("share-text"))) {
        QString device = getDevice(iface, parser);
        if (device.isEmpty())
            return 1;

        QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kdeconnect"),
                                                          QLatin1String("/modules/kdeconnect/devices/%1/share").arg(device),
                                                          QStringLiteral("org.kde.kdeconnect.device.share"),
                                                          QStringLiteral("shareText"));
        msg.setArguments(QVariantList{parser.value(QStringLiteral("share-text"))});
        blockOnReply(QDBusConnection::sessionBus().asyncCall(msg));
        QTextStream(stdout) << i18n("Shared text: %1", parser.value(QStringLiteral("share-text"))) << Qt::endl;
    } else if (parser.isSet(QStringLiteral("lock")) || parser.isSet(QStringLiteral("unlock"))) {
        QString device = getDevice(iface, parser);
        if (device.isEmpty())
            return 1;

        LockDeviceDbusInterface iface(device);
        iface.setLocked(parser.isSet(QStringLiteral("lock")));

        DeviceDbusInterface deviceIface(device);
        if (parser.isSet(QStringLiteral("lock"))) {
            QTextStream(stdout) << i18nc("device has requested to lock peer device", "Requested to lock %1.", deviceIface.name()) << Qt::endl;
        } else {
            QTextStream(stdout) << i18nc("device has requested to unlock peer device", "Requested to unlock %1.", deviceIface.name()) << Qt::endl;
        }
    } else if (parser.isSet(QStringLiteral("pair"))) {
        QString device = getDevice(iface, parser);
        if (device.isEmpty())
            return 1;

        DeviceDbusInterface dev(device);
        if (!dev.isReachable()) {
            // Device doesn't exist, go into discovery mode and wait up to 30 seconds for the device to appear
            QEventLoop wait;
            QTextStream(stderr) << i18n("waiting for device…") << Qt::endl;

            QObject::connect(&iface, &DaemonDbusInterface::deviceAdded, &iface, [&](const QString &deviceAddedId) {
                if (device == deviceAddedId) {
                    wait.quit();
                }
            });
            QTimer::singleShot(30 * 1000, &wait, &QEventLoop::quit);

            wait.exec();
        }

        if (!dev.isReachable()) {
            QTextStream(stderr) << i18n("Device not found") << Qt::endl;
        } else if (blockOnReply<bool>(dev.isPaired())) {
            QTextStream(stderr) << i18n("Already paired") << Qt::endl;
        } else {
            QTextStream(stderr) << i18n("Pair requested") << Qt::endl;
            blockOnReply(dev.requestPairing());
        }
    } else if (parser.isSet(QStringLiteral("unpair"))) {
        QString device = getDevice(iface, parser);
        if (device.isEmpty())
            return 1;

        DeviceDbusInterface dev(device);
        if (!dev.isPaired()) {
            QTextStream(stderr) << i18n("Already not paired") << Qt::endl;
        } else {
            QTextStream(stderr) << i18n("Successfully unpaired") << Qt::endl;
            blockOnReply(dev.unpair());
        }
    } else if (parser.isSet(QStringLiteral("send-clipboard"))) {
        QString device = getDevice(iface, parser);
        if (device.isEmpty())
            return 1;

        QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kdeconnect"),
                                                          QLatin1String("/modules/kdeconnect/devices/%1/clipboard").arg(device),
                                                          QStringLiteral("org.kde.kdeconnect.device.clipboard"),
                                                          QStringLiteral("sendClipboard"));
        blockOnReply(QDBusConnection::sessionBus().asyncCall(msg));
    } else if (parser.isSet(QStringLiteral("ping")) || parser.isSet(QStringLiteral("ping-msg"))) {
        QString device = getDevice(iface, parser);
        if (device.isEmpty())
            return 1;

        QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kdeconnect"),
                                                          QLatin1String("/modules/kdeconnect/devices/%1/ping").arg(device),
                                                          QStringLiteral("org.kde.kdeconnect.device.ping"),
                                                          QStringLiteral("sendPing"));
        if (parser.isSet(QStringLiteral("ping-msg"))) {
            QString message = parser.value(QStringLiteral("ping-msg"));
            msg.setArguments(QVariantList{message});
        }
        blockOnReply(QDBusConnection::sessionBus().asyncCall(msg));
    } else if (parser.isSet(QStringLiteral("send-sms"))) {
        QString device = getDevice(iface, parser);
        if (device.isEmpty())
            return 1;

        if (parser.isSet(QStringLiteral("destination"))) {
            qDBusRegisterMetaType<ConversationAddress>();
            QVariantList addresses;

            const QStringList addressList = parser.value(QStringLiteral("destination")).split(QRegularExpression(QStringLiteral("\\s+")));

            for (const QString &input : addressList) {
                ConversationAddress address(input);
                addresses << QVariant::fromValue(address);
            }

            const QString message = parser.value(QStringLiteral("send-sms"));

            const QStringList rawAttachmentUrlsList = parser.values(QStringLiteral("attachment"));

            QVariantList attachments;
            for (const QString &attachmentUrl : rawAttachmentUrlsList) {
                // TODO: Construct attachment objects from the list of Urls
                Q_UNUSED(attachmentUrl);
            }

            DeviceConversationsDbusInterface conversationDbusInterface(device);
            auto reply = conversationDbusInterface.sendWithoutConversation(addresses, message, attachments);

            reply.waitForFinished();
        } else {
            QTextStream(stderr) << i18n("error: should specify the SMS's recipient by passing --destination <phone number>") << Qt::endl;
            return 1;
        }
    } else if (parser.isSet(QStringLiteral("ring"))) {
        QString device = getDevice(iface, parser);
        if (device.isEmpty())
            return 1;

        QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kdeconnect"),
                                                          QLatin1String("/modules/kdeconnect/devices/%1/findmyphone").arg(device),
                                                          QStringLiteral("org.kde.kdeconnect.device.findmyphone"),
                                                          QStringLiteral("ring"));
        blockOnReply(QDBusConnection::sessionBus().asyncCall(msg));
    } else if (parser.isSet(QStringLiteral("send-keys"))) {
        QString device = getDevice(iface, parser);
        if (device.isEmpty())
            return 1;

        QString seq = parser.value(QStringLiteral("send-keys"));
        QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kdeconnect"),
                                                          QLatin1String("/modules/kdeconnect/devices/%1/remotekeyboard").arg(device),
                                                          QStringLiteral("org.kde.kdeconnect.device.remotekeyboard"),
                                                          QStringLiteral("sendKeyPress"));
        if (seq.trimmed() == QLatin1String("-")) {
            // from stdin
            QFile in;
            if (in.open(stdin, QIODevice::ReadOnly | QIODevice::Unbuffered)) {
                while (!in.atEnd()) {
                    QByteArray line = in.readLine(); // sanitize to ASCII-codes > 31?
                    msg.setArguments({QString::fromLatin1(line), -1, false, false, false});
                    blockOnReply(QDBusConnection::sessionBus().asyncCall(msg));
                }
                in.close();
            }
        } else {
            msg.setArguments({seq, -1, false, false, false});
            blockOnReply(QDBusConnection::sessionBus().asyncCall(msg));
        }
    } else if (parser.isSet(QStringLiteral("list-notifications"))) {
        QString device = getDevice(iface, parser);
        if (device.isEmpty())
            return 1;

        NotificationsModel notifications;
        notifications.setDeviceId(device);
        for (int i = 0, rows = notifications.rowCount(); i < rows; ++i) {
            QModelIndex idx = notifications.index(i);
            QTextStream(stdout) << "- " << idx.data(NotificationsModel::AppNameModelRole).toString() << ": "
                                << idx.data(NotificationsModel::NameModelRole).toString() << Qt::endl;
        }
    } else if (parser.isSet(QStringLiteral("list-commands"))) {
        QString device = getDevice(iface, parser);
        if (device.isEmpty())
            return 1;

        RemoteCommandsDbusInterface iface(device);
        const auto cmds = QJsonDocument::fromJson(iface.commands()).object();
        for (auto it = cmds.constBegin(), itEnd = cmds.constEnd(); it != itEnd; ++it) {
            const QJsonObject cont = it->toObject();
            QTextStream(stdout) << it.key() << ": " << cont.value(QStringLiteral("name")).toString() << ": " << cont.value(QStringLiteral("command")).toString()
                                << Qt::endl;
        }
    } else if (parser.isSet(QStringLiteral("execute-command"))) {
        QString device = getDevice(iface, parser);
        if (device.isEmpty())
            return 1;

        RemoteCommandsDbusInterface iface(device);
        blockOnReply(iface.triggerCommand(parser.value(QStringLiteral("execute-command"))));
    } else if (parser.isSet(QStringLiteral("encryption-info"))) {
        QString device = getDevice(iface, parser);
        if (device.isEmpty())
            return 1;

        DeviceDbusInterface dev(device);
        QString info = blockOnReply<QString>(dev.encryptionInfo()); // QSsl::Der = 1
        QTextStream(stdout) << info << Qt::endl;
    } else if (parser.isSet(QStringLiteral("mount"))) {
        QString device = getDevice(iface, parser);
        if (device.isEmpty())
            return 1;

        QDBusMessage msgMount = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kdeconnect"),
                                                               QLatin1String("/modules/kdeconnect/devices/%1/sftp").arg(device),
                                                               QStringLiteral("org.kde.kdeconnect.device.sftp"),
                                                               QStringLiteral("mountAndWait"));
        blockOnReply(QDBusConnection::sessionBus().asyncCall(msgMount));
    } else if (parser.isSet(QStringLiteral("get-mount-point"))) {
        QString device = getDevice(iface, parser);
        if (device.isEmpty())
            return 1;

        QDBusMessage msgMountPoint = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kdeconnect"),
                                                                    QLatin1String("/modules/kdeconnect/devices/%1/sftp").arg(device),
                                                                    QStringLiteral("org.kde.kdeconnect.device.sftp"),
                                                                    QStringLiteral("mountPoint"));
        QString mountPoint = blockOnReply<QString>(QDBusConnection::sessionBus().call(msgMountPoint));
        QTextStream(stdout) << mountPoint << Qt::endl;
    } else {
        parser.showHelp(1);
    }

    QMetaObject::invokeMethod(&app, "quit", Qt::QueuedConnection);
    return app.exec();
}
