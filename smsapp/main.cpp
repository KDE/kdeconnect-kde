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

class AppData : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString initialMessage MEMBER m_initialMessage NOTIFY initialMessageChanged)
    Q_PROPERTY(QString initialDevice MEMBER m_initialDevice NOTIFY initialDeviceChanged)

public:
    Q_SIGNAL void initialMessageChanged();
    Q_SIGNAL void initialDeviceChanged();

    QString m_initialMessage;
    QString m_initialDevice;
};

int main(int argc, char *argv[])
{
    QIcon::setFallbackThemeName(QStringLiteral("breeze"));

    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
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

    AppData data;

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);
    parser.addOption(QCommandLineOption(QStringLiteral("device"), i18n("Select a device"), i18n("id")));
    parser.addOption(QCommandLineOption(QStringLiteral("message"), i18n("Send a message"), i18n("message")));
    parser.process(app);
    aboutData.processCommandLine(&parser);

    data.m_initialMessage = parser.value(QStringLiteral("message"));
    data.m_initialDevice = parser.value(QStringLiteral("device"));

    KDBusService service(KDBusService::Unique);

    QObject::connect(&service, &KDBusService::activateRequested, &service, [&parser, &data](const QStringList &args, const QString &/*workDir*/) {
        parser.parse(args);

        data.m_initialMessage = parser.value(QStringLiteral("message"));
        data.m_initialDevice = parser.value(QStringLiteral("device"));

        Q_EMIT data.initialDeviceChanged();
        Q_EMIT data.initialMessageChanged();
    });

    qmlRegisterType<ConversationsSortFilterProxyModel>("org.kde.kdeconnect.sms", 1, 0, "QSortFilterProxyModel");
    qmlRegisterType<ConversationModel>("org.kde.kdeconnect.sms", 1, 0, "ConversationModel");
    qmlRegisterType<ConversationListModel>("org.kde.kdeconnect.sms", 1, 0, "ConversationListModel");

    qmlRegisterSingletonType<SmsHelper>("org.kde.kdeconnect.sms", 1, 0, "SmsHelper", SmsHelper::singletonProvider);

    qmlRegisterSingletonInstance<AppData>("org.kde.kdeconnect.sms", 1,0, "AppData", &data);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.addImageProvider(QStringLiteral("thumbnailsProvider"), new ThumbnailsProvider);
    engine.rootContext()->setContextProperties({
        { QStringLiteral("aboutData"), QVariant::fromValue(KAboutData::applicationData()) }
    });
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

    return app.exec();
}

#include "main.moc"
