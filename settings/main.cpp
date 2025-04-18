/*
 * SPDX-FileCopyrightText: 2018 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QQuickStyle>
#include <QStandardPaths>

#include "kdeconnect-version.h"
#include <KAboutData>
#include <KCMultiDialog>
#include <KColorSchemeManager>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedString>
#include <KWindowSystem>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("kdeconnect")));
    KAboutData aboutData(QStringLiteral("kdeconnect-settings"),
                         i18n("KDE Connect Settings"),
                         QStringLiteral(KDECONNECT_VERSION_STRING),
                         i18n("KDE Connect Settings"),
                         KAboutLicense::GPL,
                         i18n("(c) 2018-2025, KDE Connect Team"));
    aboutData.addAuthor(i18n("Nicolas Fella"), {}, QStringLiteral("nicolas.fella@gmx.de"));
    KAboutData::setApplicationData(aboutData);

#ifdef Q_OS_WIN
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
    parser.addOption(QCommandLineOption(QStringLiteral("args"), i18n("Arguments for the config module"), QStringLiteral("args")));

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    KDBusService dbusService(KDBusService::Unique);

    KCMultiDialog *dialog = new KCMultiDialog;
    dialog->addModule(KPluginMetaData(QStringLiteral("plasma/kcms/systemsettings_qwidgets/kcm_kdeconnect")), {parser.value(QStringLiteral("args"))});

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();

    QObject::connect(&dbusService, &KDBusService::activateRequested, dialog, [dialog](const QStringList &args, const QString & /*workingDir*/) {
        KWindowSystem::updateStartupId(dialog->windowHandle());
        KWindowSystem::activateWindow(dialog->windowHandle());

        QCommandLineParser parser;
        parser.addOption(QCommandLineOption(QStringLiteral("args"), i18n("Arguments for the config module"), QStringLiteral("args")));
        parser.parse(args);

        dialog->clear();
        dialog->addModule(KPluginMetaData(QStringLiteral("plasma/kcms/systemsettings_qwidgets/kcm_kdeconnect")), {parser.value(QStringLiteral("args"))});
    });

    app.setQuitOnLastWindowClosed(true);

    return app.exec();
}
