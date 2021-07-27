/**
 * SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "conversationmodel.h"
#include "conversationlistmodel.h"
#include "conversationssortfilterproxymodel.h"
#include "thumbnailsprovider.h"
#include "kdeconnect-version.h"

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QCommandLineParser>
#include <QQmlContext>
#include <QQuickStyle>
#include <KAboutData>
#include <KLocalizedString>
#include <KLocalizedContext>
#include <KDBusService>
#include <KColorSchemeManager>

#include "smshelper.h"

int main(int argc, char *argv[])
{
    QIcon::setFallbackThemeName(QStringLiteral("breeze"));

    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("kdeconnect")));
    KAboutData aboutData(QStringLiteral("kdeconnect.sms"),
                         i18n("KDE Connect SMS"),
                         QStringLiteral(KDECONNECT_VERSION_STRING),
                         i18n("SMS Instant Messaging"),
                         KAboutLicense::GPL_V3,
                         i18n("(C) 2018-2019, KDE Connect Team"));
    aboutData.addAuthor(i18n("Simon Redman"), {}, QStringLiteral("simon@ergotech.com"));
    aboutData.addAuthor(i18n("Aleix Pol Gonzalez"), {}, QStringLiteral("aleixpol@kde.org"));
    aboutData.addAuthor(i18n("Nicolas Fella"), {}, QStringLiteral("nicolas.fella@gmx.de"));
    KAboutData::setApplicationData(aboutData);

#ifdef Q_OS_WIN
    KColorSchemeManager manager;
    QApplication::setStyle(QStringLiteral("breeze"));
#endif

    // Default to org.kde.desktop style unless the user forces another style
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }

    QString initialMessage, deviceid;

    {
        QCommandLineParser parser;
        aboutData.setupCommandLine(&parser);
        parser.addOption(QCommandLineOption(QStringLiteral("device"), i18n("Select a device"), i18n("id")));
        parser.addOption(QCommandLineOption(QStringLiteral("message"), i18n("Send a message"), i18n("message")));
        parser.process(app);
        aboutData.processCommandLine(&parser);

        initialMessage = parser.value(QStringLiteral("message"));
        deviceid = parser.value(QStringLiteral("device"));
    }

    KDBusService service(KDBusService::Unique);

    qmlRegisterType<ConversationsSortFilterProxyModel>("org.kde.kdeconnect.sms", 1, 0, "QSortFilterProxyModel");
    qmlRegisterType<ConversationModel>("org.kde.kdeconnect.sms", 1, 0, "ConversationModel");
    qmlRegisterType<ConversationListModel>("org.kde.kdeconnect.sms", 1, 0, "ConversationListModel");

    qmlRegisterSingletonType<SmsHelper>("org.kde.kdeconnect.sms", 1, 0, "SmsHelper", SmsHelper::singletonProvider);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.addImageProvider(QStringLiteral("thumbnailsProvider"), new ThumbnailsProvider);
    engine.rootContext()->setContextProperties({
        { QStringLiteral("initialMessage"), initialMessage },
        { QStringLiteral("initialDevice"), deviceid },
        { QStringLiteral("aboutData"), QVariant::fromValue(KAboutData::applicationData()) }
    });
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

    return app.exec();
}
