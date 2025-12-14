/*
 * SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QAbstractButton>
#include <QApplication>
#include <QComboBox>
#include <QCommandLineParser>
#include <QDBusMessage>
#include <QDialog>
#include <QFileDialog>
#include <QIcon>
#include <QMessageBox>
#include <QQuickStyle>
#include <QTextStream>
#include <QUrl>

#include <KAboutData>
#include <KColorSchemeManager>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedString>
#include <KUrlRequester>

#include <dbushelper.h>

#include "dbusinterfaces/dbushelpers.h"
#include "dbusinterfaces/dbusinterfaces.h"
#include "kdeconnect-version.h"
#include "models/devicesmodel.h"
#include "models/devicespluginfilterproxymodel.h"
#include "ui_dialog.h"

/**
 * Show only devices that can be shared to
 */

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("kdeconnect")));
    const QString description = i18n("KDE Connect URL handler");
    KAboutData aboutData(QStringLiteral("kdeconnect.handler"),
                         description,
                         QStringLiteral(KDECONNECT_VERSION_STRING),
                         description,
                         KAboutLicense::GPL,
                         i18n("© 2015–2025 KDE Connect Team"));
    aboutData.addAuthor(i18n("Aleix Pol Gonzalez"), {}, QStringLiteral("aleixpol@kde.org"));
    aboutData.addAuthor(i18n("Albert Vaca Cintora"), {}, QStringLiteral("albertvaka@gmail.org"));
    aboutData.setProgramLogo(QIcon::fromTheme(QStringLiteral("kdeconnect")));
    KAboutData::setApplicationData(aboutData);

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

    KDBusService dbusService(KDBusService::Unique);

    QUrl urlToShare;
    bool open;
    QString deviceId;
    {
        QCommandLineParser parser;
        parser.addPositionalArgument(QStringLiteral("url"), i18n("URL to share"));
        parser.addOption(QCommandLineOption(QStringLiteral("device"), i18n("Select a device"), i18n("id")));
        parser.addOption(QCommandLineOption(QStringLiteral("open"), i18n("Automatically open the file on the remote device, when supported")));
        aboutData.setupCommandLine(&parser);
        parser.process(app);
        aboutData.processCommandLine(&parser);
        if (parser.positionalArguments().count() == 1) {
            urlToShare = QUrl::fromUserInput(parser.positionalArguments().constFirst(), QDir::currentPath(), QUrl::AssumeLocalFile);
        }
        open = parser.isSet(QStringLiteral("open"));
        deviceId = parser.value(QStringLiteral("device"));
    }

    DevicesModel model;
    model.setDisplayFilter(DevicesModel::Paired | DevicesModel::Reachable);
    DevicesPluginFilterProxyModel proxyModel;
    proxyModel.setPluginFilter(QStringLiteral("kdeconnect_share"));
    proxyModel.setSourceModel(&model);

    QDialog dialog;

    Ui::Dialog uidialog;
    uidialog.setupUi(&dialog);
    uidialog.devicePicker->setModel(&proxyModel);

    if (!deviceId.isEmpty()) {
        // This is done on rowsInserted because the model isn't populated yet
        QObject::connect(&model, &QAbstractItemModel::rowsInserted, &app, [&uidialog, &deviceId, &proxyModel](const QModelIndex &parent, int first) {
            Q_UNUSED(parent);
            /**
             * We want to run this only once, but on startup the data is populated, removed then repopulated (rowsInserted() -> rowsRemoved() ->
             * rowsInserted()). Thus by running only when there were no devices previously, it ensures that the user's selection doesn't change without them
             * realising.
             */
            if (first == 0) {
                uidialog.devicePicker->setCurrentIndex(proxyModel.rowForDevice(deviceId));
            }
        });
    }

    KUrlRequester *urlRequester = new KUrlRequester(&dialog);
    urlRequester->setStartDir(QUrl::fromLocalFile(QDir::homePath()));
    urlRequester->setFocus();
    uidialog.urlPickerLayout->addWidget(urlRequester);
    urlRequester->setPlaceholderText(i18nc("Placeholder for input field that should contain a file/URL to share", "Local file or web URL"));
    if (!urlToShare.isEmpty()) {
        uidialog.urlPickerLabel->setVisible(false);
        urlRequester->setVisible(false);
        urlRequester->setUrl(urlToShare);

        QString displayUrl;
        if (urlToShare.scheme() == QLatin1String("tel")) {
            displayUrl = urlToShare.toDisplayString(QUrl::RemoveScheme);
            uidialog.devicePickerLabel->setText(i18n("Device to call %1 with:", displayUrl));
        } else if (urlToShare.isLocalFile() && open) {
            displayUrl = urlToShare.toDisplayString(QUrl::PreferLocalFile);
            uidialog.devicePickerLabel->setText(i18n("Device to open %1 on:", displayUrl));
        } else if (urlToShare.scheme() == QLatin1String("sms")) {
            displayUrl = urlToShare.toDisplayString(QUrl::PreferLocalFile);
            uidialog.devicePickerLabel->setText(i18n("Device to send a SMS with:"));
        } else {
            displayUrl = urlToShare.toDisplayString(QUrl::PreferLocalFile);
            uidialog.devicePickerLabel->setText(i18n("Device to send %1 to:", displayUrl));
        }
    }

    if (dialog.exec() == QDialog::Accepted) {
        const QUrl url = urlRequester->url();
        const int currentDeviceIndex = uidialog.devicePicker->currentIndex();
        if (!url.isEmpty() && currentDeviceIndex >= 0) {
            const QString device = proxyModel.index(currentDeviceIndex, 0).data(DevicesModel::IdModelRole).toString();
            const QString action = open && url.isLocalFile() ? QStringLiteral("openFile") : QStringLiteral("shareUrl");

            QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kdeconnect"),
                                                              QLatin1String("/modules/kdeconnect/devices/%1/share").arg(device),
                                                              QStringLiteral("org.kde.kdeconnect.device.share"),
                                                              action);
            msg.setArguments({url.toString()});
            blockOnReply(QDBusConnection::sessionBus().asyncCall(msg));
            return 0;
        } else {
            QMessageBox::critical(nullptr, description, i18n("Couldn't share %1", url.toString()));
            return 1;
        }
    } else {
        return 1;
    }
}
