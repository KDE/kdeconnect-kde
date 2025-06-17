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
#include <QQuickStyle>
#include <QSessionManager>
#include <QStandardPaths>
#include <QTimer>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

#include <KAboutData>
#include <KColorSchemeManager>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedString>

#include <dbushelper.h>

#include "desktop_daemon.h"
#include "kdeconnect-version.h"

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
                         KAboutLicense::GPL,
                         i18n("© 2015–2025 KDE Connect Team"));
    KAboutData::setApplicationData(aboutData);
    app.setQuitOnLastWindowClosed(false);

#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    // Ensure we have a suitable color theme set for light/dark mode. KColorSchemeManager implicitly applies
    // a suitable default theme.
    KColorSchemeManager::instance();
    // Force breeze style to ensure coloring works consistently in dark mode. Specifically tab colors have
    // troubles on windows.
    QApplication::setStyle(QStringLiteral("breeze"));
    // Force breeze icon theme to ensure we can correctly adapt icons to color changes WRT dark/light mode.
    // Without this we may end up with hicolor and fail to support icon recoloring.
    QIcon::setThemeName(QStringLiteral("breeze"));
#else
    QIcon::setFallbackThemeName(QStringLiteral("breeze"));
#endif

    // Default to org.kde.desktop style unless the user forces another style
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }

    KCrash::initialize();

    QCommandLineParser parser;
    QCommandLineOption replaceOption({QStringLiteral("replace")}, i18n("Replace an existing instance"));
    parser.addOption(replaceOption);
    aboutData.setupCommandLine(&parser);

    parser.process(app);

    aboutData.processCommandLine(&parser);

    KDBusService::StartupOptions flags = KDBusService::Unique;
    if (parser.isSet(replaceOption)) {
        flags |= KDBusService::Replace;
    }
    KDBusService dbusService(flags);

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

    return app.exec();
}
