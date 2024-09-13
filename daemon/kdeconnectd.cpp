/**
 * SPDX-FileCopyrightText: 2014 Yuri Samoilenko <kinnalru@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDBusMessage>
#include <QIcon>
#include <QProcess>
#include <QSessionManager>
#include <QStandardPaths>
#include <QTimer>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

#include <KAboutData>
#include <KCrash>
#include <KDBusService>
#include <KIO/Global>
#include <KIO/JobTracker>
#include <KLocalizedString>
#include <KNotification>
#include <KWindowSystem>

#include <dbushelper.h>

#include "core/backends/pairinghandler.h"
#include "core/device.h"
#include "core/openconfig.h"
#include "desktop_daemon.h"
#include "kdeconnect-version.h"
#include "kdeconnectd_debug.h"

// Copied from plasma-workspace/libkworkspace/kworkspace.cpp
static void detectPlatform(int argc, char **argv)
{
    if (qEnvironmentVariableIsSet("QT_QPA_PLATFORM")) {
        return;
    }
    for (int i = 0; i < argc; i++) {
        if (qstrcmp(argv[i], "-platform") == 0 || qstrcmp(argv[i], "--platform") == 0 || QByteArray(argv[i]).startsWith("-platform=")
            || QByteArray(argv[i]).startsWith("--platform=")) {
            return;
        }
    }
    const QByteArray sessionType = qgetenv("XDG_SESSION_TYPE");
    if (sessionType.isEmpty()) {
        return;
    }
    if (qstrcmp(sessionType.constData(), "wayland") == 0) {
        qputenv("QT_QPA_PLATFORM", "wayland");
    } else if (qstrcmp(sessionType.constData(), "x11") == 0) {
        qputenv("QT_QPA_PLATFORM", "xcb");
    }
}

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    // If ran from a console, redirect the output there
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
#endif

    detectPlatform(argc, argv);
    QGuiApplication::setQuitLockEnabled(false);

    QApplication app(argc, argv);
    KAboutData aboutData(QStringLiteral("kdeconnect.daemon"),
                         i18n("KDE Connect Daemon"),
                         QStringLiteral(KDECONNECT_VERSION_STRING),
                         i18n("KDE Connect Daemon"),
                         KAboutLicense::GPL);
    KAboutData::setApplicationData(aboutData);
    app.setQuitOnLastWindowClosed(false);

    KCrash::initialize();

    QCommandLineParser parser;
    QCommandLineOption replaceOption({QStringLiteral("replace")}, i18n("Replace an existing instance"));
    parser.addOption(replaceOption);
#ifdef Q_OS_MAC
    QCommandLineOption macosPrivateDBusOption({QStringLiteral("use-private-dbus")},
                                              i18n("Launch a private D-Bus daemon with kdeconnectd (macOS test-purpose only)"));
    parser.addOption(macosPrivateDBusOption);
#endif
    aboutData.setupCommandLine(&parser);

    parser.process(app);
#ifdef Q_OS_MAC
    if (parser.isSet(macosPrivateDBusOption)) {
        DBusHelper::launchDBusDaemon();
    }
#endif
    aboutData.processCommandLine(&parser);
    if (parser.isSet(replaceOption)) {
        auto message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kdeconnect"),
                                                      QStringLiteral("/MainApplication"),
                                                      QStringLiteral("org.qtproject.Qt.QCoreApplication"),
                                                      QStringLiteral("quit"));
        QDBusConnection::sessionBus().call(message); // deliberately block until it's done, so we register the name after the app quits
    }

    KDBusService dbusService(KDBusService::Unique);

    DesktopDaemon daemon;

#ifdef Q_OS_WIN
    // make sure indicator shows up in the tray whenever daemon is spawned
    QProcess::startDetached(QStringLiteral("kdeconnect-indicator.exe"), QStringList());
#endif

    // kdeconnectd is autostarted, so disable session management to speed up startup
    auto disableSessionManagement = [](QSessionManager &sm) {
        sm.setRestartHint(QSessionManager::RestartNever);
    };
    QObject::connect(&app, &QGuiApplication::commitDataRequest, disableSessionManagement);
    QObject::connect(&app, &QGuiApplication::saveStateRequest, disableSessionManagement);

    qSetMessagePattern(QStringLiteral("%{time} %{category}: %{message}"));

    return app.exec();
}
