/**
 * SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "conversationlistmodel.h"
#include "conversationmodel.h"
#include "conversationssortfilterproxymodel.h"
#include "kdeconnect-version.h"
#include "thumbnailsprovider.h"

#include <KAboutData>
#include <KColorSchemeManager>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedContext>
#include <KLocalizedString>

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "smshelper.h"

class AppData : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString initialMessage MEMBER m_initialMessage NOTIFY initialMessageChanged)
    Q_PROPERTY(QString deviceId MEMBER m_deviceId NOTIFY deviceIdChanged)

public:
    Q_SIGNAL void initialMessageChanged();
    Q_SIGNAL void deviceIdChanged();

    QString m_initialMessage;
    QString m_deviceId;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("kdeconnect-sms");
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("kdeconnect")));
    KAboutData aboutData(QStringLiteral("kdeconnect.sms"),
                         i18n("KDE Connect SMS"),
                         QStringLiteral(KDECONNECT_VERSION_STRING),
                         i18n("SMS Instant Messaging"),
                         KAboutLicense::GPL_V3,
                         i18n("(C) 2018-2022, KDE Connect Team"));
    aboutData.addAuthor(i18n("Simon Redman"), {}, QStringLiteral("simon@ergotech.com"));
    aboutData.addAuthor(i18n("Aleix Pol Gonzalez"), {}, QStringLiteral("aleixpol@kde.org"));
    aboutData.addAuthor(i18n("Nicolas Fella"), {}, QStringLiteral("nicolas.fella@gmx.de"));
    aboutData.setBugAddress(QStringLiteral("https://bugs.kde.org/enter_bug.cgi?product=kdeconnect&component=messaging-application").toUtf8());
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

    AppData data;

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);
    parser.addOption(QCommandLineOption(QStringLiteral("device"), i18n("Select a device"), i18n("id")));
    parser.addOption(QCommandLineOption(QStringLiteral("message"), i18n("Send a message"), i18n("message")));
    parser.process(app);
    aboutData.processCommandLine(&parser);

    data.m_initialMessage = parser.value(QStringLiteral("message"));
    data.m_deviceId = parser.value(QStringLiteral("device"));

    KDBusService service(KDBusService::Unique);

    QObject::connect(&service, &KDBusService::activateRequested, &service, [&parser, &data](const QStringList &args, const QString & /*workDir*/) {
        parser.parse(args);

        data.m_initialMessage = parser.value(QStringLiteral("message"));
        data.m_deviceId = parser.value(QStringLiteral("device"));

        Q_EMIT data.deviceIdChanged();
        Q_EMIT data.initialMessageChanged();
    });

    qmlRegisterType<ConversationsSortFilterProxyModel>("org.kde.kdeconnect.sms", 1, 0, "QSortFilterProxyModel");
    qmlRegisterType<ConversationModel>("org.kde.kdeconnect.sms", 1, 0, "ConversationModel");
    qmlRegisterType<ConversationListModel>("org.kde.kdeconnect.sms", 1, 0, "ConversationListModel");

    qmlRegisterSingletonType<SmsHelper>("org.kde.kdeconnect.sms", 1, 0, "SmsHelper", SmsHelper::singletonProvider);

    qmlRegisterSingletonInstance<AppData>("org.kde.kdeconnect.sms", 1, 0, "AppData", &data);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.addImageProvider(QStringLiteral("thumbnailsProvider"), new ThumbnailsProvider);
    engine.loadFromModule("org.kde.kdeconnect.sms", "Main");

    return app.exec();
}

#include "main.moc"
