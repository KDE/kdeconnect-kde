/**
 * Copyright 2014 Yuri Samoilenko <kinnalru@gmail.com>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <QApplication>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QLoggingCategory>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QSessionManager>

#include <KAboutData>
#include <KDBusService>
#include <KNotification>
#include <KLocalizedString>
#include <KIO/AccessManager>

#include "core/daemon.h"
#include "core/device.h"
#include "core/backends/pairinghandler.h"
#include "kdeconnect-version.h"

Q_DECLARE_LOGGING_CATEGORY(KDECONNECT_DAEMON)
Q_LOGGING_CATEGORY(KDECONNECT_DAEMON, "kdeconnect.daemon")

class DesktopDaemon : public Daemon
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.daemon")
public:
    DesktopDaemon(QObject* parent = nullptr)
        : Daemon(parent)
        , m_nam(nullptr)
    {}

    void askPairingConfirmation(Device* device) override
    {
        KNotification* notification = new KNotification(QStringLiteral("pairingRequest"));
        notification->setIconName(QStringLiteral("dialog-information"));
        notification->setComponentName(QStringLiteral("kdeconnect"));
        notification->setText(i18n("Pairing request from %1", device->name().toHtmlEscaped()));
        notification->setActions(QStringList() << i18n("Accept") << i18n("Reject"));
//         notification->setTimeout(PairingHandler::pairingTimeoutMsec());
        connect(notification, &KNotification::action1Activated, device, &Device::acceptPairing);
        connect(notification, &KNotification::action2Activated, device, &Device::rejectPairing);
        notification->sendEvent();
    }

    void reportError(const QString & title, const QString & description) override
    {
        qCWarning(KDECONNECT_DAEMON) << title << ":" << description;
        KNotification::event(KNotification::Error, title, description);
    }

    QNetworkAccessManager* networkAccessManager() override
    {
        if (!m_nam) {
            m_nam = new KIO::AccessManager(this);
        }
        return m_nam;
    }

    Q_SCRIPTABLE void sendSimpleNotification(const QString &eventId, const QString &title, const QString &text, const QString &iconName) override
    {
        KNotification* notification = new KNotification(eventId); //KNotification::Persistent
        notification->setIconName(iconName);
        notification->setComponentName(QStringLiteral("kdeconnect"));
        notification->setTitle(title);
        notification->setText(text);
        notification->sendEvent();
    }

    void quit() override {
        QApplication::quit();
    }

private:
    QNetworkAccessManager* m_nam;
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    KAboutData aboutData(
        QStringLiteral("org.kde.kdeconnect.daemon"),
        i18n("KDE Connect Daemon"),
        QStringLiteral(KDECONNECT_VERSION_STRING),
        i18n("KDE Connect Daemon"),
        KAboutLicense::GPL
    );
    KAboutData::setApplicationData(aboutData);
    app.setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    QCommandLineOption replaceOption({QStringLiteral("replace")}, i18n("Replace an existing instance"));
    parser.addOption(replaceOption);
    aboutData.setupCommandLine(&parser);

    parser.process(app);
    aboutData.processCommandLine(&parser);
    if (parser.isSet(replaceOption)) {
        auto message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kdeconnectd"),
                                                    QStringLiteral("/MainApplication"),
                                                    QStringLiteral("org.qtproject.Qt.QCoreApplication"),
                                                    QStringLiteral("quit"));
        QDBusConnection::sessionBus().call(message); //deliberately block until it's done, so we register the name after the app quits
    }

    KDBusService dbusService(KDBusService::Unique);

    DesktopDaemon daemon;

    // kdeconnectd is autostarted, so disable session management to speed up startup
    auto disableSessionManagement = [](QSessionManager &sm) {
        sm.setRestartHint(QSessionManager::RestartNever);
    };
    QObject::connect(&app, &QGuiApplication::commitDataRequest, disableSessionManagement);
    QObject::connect(&app, &QGuiApplication::saveStateRequest, disableSessionManagement);

    return app.exec();
}

#include "kdeconnectd.moc"
