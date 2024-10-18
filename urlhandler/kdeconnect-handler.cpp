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

#include "kdeconnect-version.h"
#include "ui_dialog.h"
#include <interfaces/dbushelpers.h>
#include <interfaces/dbusinterfaces.h>
#include <interfaces/devicesmodel.h>
#include <interfaces/devicespluginfilterproxymodel.h>

/**
 * Show only devices that can be shared to
 */

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("kdeconnect")));
    const QString description = i18n("KDE Connect URL handler");
    KAboutData about(QStringLiteral("kdeconnect.handler"),
                     description,
                     QStringLiteral(KDECONNECT_VERSION_STRING),
                     description,
                     KAboutLicense::GPL,
                     i18n("(C) 2017 Aleix Pol Gonzalez"));
    about.addAuthor(QStringLiteral("Aleix Pol Gonzalez"), QString(), QStringLiteral("aleixpol@kde.org"));
    about.setProgramLogo(QIcon(QStringLiteral(":/icons/kdeconnect/kdeconnect.svg")));
    KAboutData::setApplicationData(about);

    KCrash::initialize();

    KDBusService dbusService(KDBusService::Unique);

#ifdef Q_OS_WIN
    KColorSchemeManager::instance();
    QApplication::setStyle(QStringLiteral("breeze"));
#endif

    // Default to org.kde.desktop style unless the user forces another style
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }

    QUrl urlToShare;
    bool open;
    QString deviceId;
    {
        QCommandLineParser parser;
        parser.addPositionalArgument(QStringLiteral("url"), i18n("URL to share"));
        parser.addOption(QCommandLineOption(QStringLiteral("device"), i18n("Select a device"), i18n("id")));
        parser.addOption(QCommandLineOption(QStringLiteral("open"), QStringLiteral("Open the file on the remote device")));
        about.setupCommandLine(&parser);
        parser.process(app);
        about.processCommandLine(&parser);
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

    uidialog.openOnPeerCheckBox->setChecked(open);

    KUrlRequester *urlRequester = new KUrlRequester(&dialog);
    urlRequester->setStartDir(QUrl::fromLocalFile(QDir::homePath()));
    uidialog.urlHorizontalLayout->addWidget(urlRequester);

    QObject::connect(uidialog.sendUrlRadioButton, &QRadioButton::toggled, [&uidialog, urlRequester](const bool checked) {
        if (checked) {
            urlRequester->setPlaceholderText(i18n("Enter URL here"));
            urlRequester->button()->setVisible(false);
            uidialog.openOnPeerCheckBox->setVisible(false);
        }
    });

    QObject::connect(uidialog.sendFileRadioButton, &QAbstractButton::toggled, [&uidialog, urlRequester](const bool checked) {
        if (checked) {
            urlRequester->setPlaceholderText(i18n("Enter file location here"));
            urlRequester->button()->setVisible(true);
            uidialog.openOnPeerCheckBox->setVisible(true);
        }
    });

    QObject::connect(urlRequester, &KUrlRequester::textChanged, [&urlRequester, &uidialog]() {
        QUrl fileUrl(urlRequester->url());
        bool isLocalFileUrl = false;
        if (fileUrl.isLocalFile()) {
            QFileInfo fileInfo(fileUrl.toLocalFile());
            isLocalFileUrl = fileInfo.exists() && fileInfo.isFile(); // we don't support sending directories yet!
        }
        uidialog.sendFileRadioButton->setChecked(isLocalFileUrl);
        uidialog.sendUrlRadioButton->setChecked(!isLocalFileUrl);
    });

    if (!urlToShare.isEmpty()) {
        uidialog.sendUrlRadioButton->setVisible(false);
        uidialog.sendFileRadioButton->setVisible(false);
        urlRequester->setVisible(false);

        QString displayUrl;
        if (urlToShare.scheme() == QLatin1String("tel")) {
            displayUrl = urlToShare.toDisplayString(QUrl::RemoveScheme);
            uidialog.label->setText(i18n("Device to call %1 with:", displayUrl));
        } else if (urlToShare.isLocalFile() && open) {
            displayUrl = urlToShare.toDisplayString(QUrl::PreferLocalFile);
            uidialog.label->setText(i18n("Device to open %1 on:", displayUrl));
        } else if (urlToShare.scheme() == QLatin1String("sms")) {
            displayUrl = urlToShare.toDisplayString(QUrl::PreferLocalFile);
            uidialog.label->setText(i18n("Device to send a SMS with:"));
        } else {
            displayUrl = urlToShare.toDisplayString(QUrl::PreferLocalFile);
            uidialog.label->setText(i18n("Device to send %1 to:", displayUrl));
        }
    }

    if (open || urlToShare.isLocalFile()) {
        uidialog.sendFileRadioButton->setChecked(true);
        urlRequester->setUrl(QUrl(urlToShare.toLocalFile()));

    } else {
        uidialog.sendUrlRadioButton->setChecked(true);
        urlRequester->setUrl(urlToShare);
    }

    if (dialog.exec() == QDialog::Accepted) {
        const QUrl url = urlRequester->url();
        open = uidialog.openOnPeerCheckBox->isChecked();
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
