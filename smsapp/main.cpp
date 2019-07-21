/**
 * Copyright (C) 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 * Copyright (C) 2018 Simon Redman <simon@ergotech.com>
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

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    KAboutData aboutData(QStringLiteral("kdeconnect.sms"), i18n("SMS Instant Messaging"), QStringLiteral(KDECONNECT_VERSION_STRING), i18n("KDE Connect SMS"), KAboutLicense::GPL, i18n("(c) 2018, Aleix Pol Gonzalez"));
    aboutData.addAuthor(i18n("Simon Redman"), {}, QStringLiteral("simon@ergotech.com"));
    aboutData.addAuthor(i18n("Aleix Pol Gonzalez"), {}, QStringLiteral("aleixpol@kde.org"));
    aboutData.addAuthor(i18n("Nicolas Fella"), {}, QStringLiteral("nicolas.fella@gmx.de"));
    KAboutData::setApplicationData(aboutData);

    QString initialMessage;

    {
        QCommandLineParser parser;
        aboutData.setupCommandLine(&parser);
        parser.addVersionOption();
        parser.addHelpOption();
        parser.addOption(QCommandLineOption(QStringLiteral("message"), i18n("Send a message"), i18n("message")));
        parser.process(app);
        aboutData.processCommandLine(&parser);

        if (parser.isSet(QStringLiteral("message"))) {
            initialMessage = parser.value(QStringLiteral("message"));
        }
    }

    KDBusService service(KDBusService::Unique);

    qmlRegisterType<QAbstractItemModel>();
    qmlRegisterType<OurSortFilterProxyModel>("org.kde.kdeconnect.sms", 1, 0, "QSortFilterProxyModel");
    qmlRegisterType<ConversationModel>("org.kde.kdeconnect.sms", 1, 0, "ConversationModel");
    qmlRegisterType<ConversationListModel>("org.kde.kdeconnect.sms", 1, 0, "ConversationListModel");

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.rootContext()->setContextProperty(QStringLiteral("_initialMessage"), QVariant(initialMessage));
    engine.rootContext()->setContextProperty(QStringLiteral("aboutData"), QVariant::fromValue(KAboutData::applicationData()));
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

    return app.exec();
}
