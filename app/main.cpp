/**
 * SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QStandardPaths>

#include "kdeconnect-version.h"
#include <KAboutData>
#include <KColorSchemeManager>
#include <KCrash>
#include <KLocalizedContext>
#include <KLocalizedString>

int main(int argc, char *argv[])
{
    QIcon::setFallbackThemeName(QStringLiteral("breeze"));

    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("kdeconnect-app");
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("kdeconnect")));
    KAboutData aboutData(QStringLiteral("kdeconnect.app"),
                         i18n("KDE Connect"),
                         QStringLiteral(KDE_CONNECT_VERSION_STRING),
                         i18n("KDE Connect"),
                         KAboutLicense::GPL,
                         i18n("(c) 2015, Aleix Pol Gonzalez"));
    aboutData.addAuthor(i18n("Aleix Pol Gonzalez"), i18n("Maintainer"), QStringLiteral("aleixpol@kde.org"));
    aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));
    aboutData.setBugAddress("https://bugs.kde.org/enter_bug.cgi?product=kdeconnect&component=common");
    KAboutData::setApplicationData(aboutData);

    KCrash::initialize();

#ifdef Q_OS_WIN
    KColorSchemeManager::instance();
    QApplication::setStyle(QStringLiteral("breeze"));
#endif

    // Default to org.kde.desktop style unless the user forces another style
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }

    QString urlToShare;
    {
        QCommandLineParser parser;
        parser.addPositionalArgument(QStringLiteral("url"), i18n("URL to share"));
        aboutData.setupCommandLine(&parser);
        parser.process(app);
        aboutData.processCommandLine(&parser);
        if (parser.positionalArguments().count() == 1) {
            urlToShare = parser.positionalArguments().constFirst();
            const QString kdeconnectHandlerExecutable =
                QStandardPaths::findExecutable(QStringLiteral("kdeconnect-handler"), {QCoreApplication::applicationDirPath()});
            if (!kdeconnectHandlerExecutable.isEmpty()) {
                QProcess::startDetached(kdeconnectHandlerExecutable, {urlToShare});
                return 0; // exit the app once kdeconnect-handler is started
            }
        }
    }

    qmlRegisterSingletonType("org.kde.kdeconnect.app", 1, 0, "About", [](QQmlEngine *engine, QJSEngine *) -> QJSValue {
        return engine->toScriptValue(KAboutData::applicationData());
    });

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.loadFromModule("org.kde.kdeconnect.app", "Main");

    return app.exec();
}
