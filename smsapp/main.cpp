/*
 * This file is part of KDE Telepathy Chat
 *
 * Copyright (C) 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "conversationmodel.h"
#include "conversationlistmodel.h"
#include "kdeconnect-version.h"

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QCommandLineParser>
#include <QQmlContext>
#include <KAboutData>
#include <KLocalizedString>
#include <KLocalizedContext>
#include <KDBusService>
#include <QtQml>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    KAboutData aboutData("org.kde.kdeconnect.sms", i18n("SMS Instant Messaging"), QStringLiteral(KDECONNECT_VERSION_STRING), i18n("KDE Connect SMS"), KAboutLicense::GPL, i18n("(c) 2018, Aleix Pol Gonzalez"));
    aboutData.addAuthor(i18n("Aleix Pol Gonzalez"), {}, "aleixpol@kde.org");
    KAboutData::setApplicationData(aboutData);

    {
        QCommandLineParser parser;
        aboutData.setupCommandLine(&parser);
        parser.addVersionOption();
        parser.addHelpOption();
        parser.process(app);
        aboutData.processCommandLine(&parser);
    }

    KDBusService service(KDBusService::Unique);

    qmlRegisterType<OurSortFilterProxyModel>("org.kde.kdeconnect.sms", 1, 0, "QSortFilterProxyModel");
    qmlRegisterType<ConversationModel>("org.kde.kdeconnect.sms", 1, 0, "ConversationModel");
    qmlRegisterType<ConversationListModel>("org.kde.kdeconnect.sms", 1, 0, "ConversationListModel");

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.load(QUrl("qrc:/qml/main.qml"));

    return app.exec();
}
