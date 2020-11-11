/**
 * SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QCommandLineParser>
#include <QQmlContext>
#include <QStandardPaths>

#include <KAboutData>
#include <KLocalizedString>
#include <KLocalizedContext>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("kdeconnect"),
        QIcon(QStandardPaths::locate(QStandardPaths::AppLocalDataLocation, QStringLiteral("icons/hicolor/scalable/apps/kdeconnect.svg")))));
    KAboutData aboutData(QStringLiteral("kdeconnect.app"), i18n("KDE Connect"), QStringLiteral("1.0"), i18n("KDE Connect"), KAboutLicense::GPL, i18n("(c) 2015, Aleix Pol Gonzalez"));
    aboutData.addAuthor(i18n("Aleix Pol Gonzalez"), i18n("Maintainer"), QStringLiteral("aleixpol@kde.org"));
    KAboutData::setApplicationData(aboutData);

    {
        QCommandLineParser parser;
        aboutData.setupCommandLine(&parser);
        parser.process(app);
        aboutData.processCommandLine(&parser);
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

    return app.exec();
}
