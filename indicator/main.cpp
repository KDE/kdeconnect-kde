/*
 * SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QApplication>
#include <QIcon>
#include <QPointer>
#include <QProcess>
#include <QQuickStyle>
#include <QThread>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

#include <QSystemTrayIcon>

#include <KAboutData>
#include <KCMultiDialog>
#include <KColorSchemeManager>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedString>

#include "deviceindicator.h"
#include "interfaces/dbusinterfaces.h"
#include "interfaces/devicesmodel.h"
#include "interfaces/devicessortproxymodel.h"
#include "kdeconnect-version.h"

#include <dbushelper.h>

#include "indicatorhelper.h"

int main(int argc, char **argv)
{
#ifdef Q_OS_WIN
    // If ran from a console, redirect the output there
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
#endif

    QApplication app(argc, argv);

    IndicatorHelper helper;
    helper.startDaemon();

    KAboutData aboutData(QStringLiteral("kdeconnect-indicator"),
                         i18n("KDE Connect Indicator"),
                         QStringLiteral(KDECONNECT_VERSION_STRING),
                         i18n("KDE Connect Indicator tool"),
                         KAboutLicense::GPL,
                         i18n("(c) 2016-2025, KDE Connect Team"));
    aboutData.addAuthor(i18n("Aleix Pol Gonzalez"), {}, QStringLiteral("aleixpol@kde.org"));
    aboutData.addAuthor(i18n("Albert Vaca Cintora"), {}, QStringLiteral("albertvaka@kde.org"));
    aboutData.setProgramLogo(QIcon::fromTheme(QStringLiteral("kdeconnect")));
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

    KDBusService dbusService(KDBusService::Unique);

    DevicesModel model;
    model.setDisplayFilter(DevicesModel::Reachable | DevicesModel::Paired);
    DevicesSortProxyModel proxyModel;
    proxyModel.setSourceModel(&model);
    QMenu *menu = new QMenu;

    QPointer<KCMultiDialog> dialog;

    DaemonDbusInterface iface;

    auto refreshMenu = [&iface, &proxyModel, &menu, &dialog]() {
        menu->clear();
        auto configure = menu->addAction(QIcon::fromTheme(QStringLiteral("configure")), i18n("Configure..."));
        QObject::connect(configure, &QAction::triggered, configure, [&dialog]() {
            if (dialog == nullptr) {
                dialog = new KCMultiDialog;
                dialog->addModule(KPluginMetaData(QStringLiteral("plasma/kcms/systemsettings_qwidgets/kcm_kdeconnect")));
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                dialog->show();
                dialog->raise();
            } else {
                dialog->raise();
                dialog->activateWindow();
            }
        });
        for (int i = 0, count = proxyModel.rowCount(); i < count; ++i) {
            QObject *deviceObject = proxyModel.data(proxyModel.index(i, 0), DevicesModel::DeviceRole).value<QObject *>();
            DeviceDbusInterface *device = qobject_cast<DeviceDbusInterface *>(deviceObject);
            if (device == nullptr)
                continue;

            auto indicator = new DeviceIndicator(device);
            QObject::connect(device, &DeviceDbusInterface::destroyed, indicator, &QObject::deleteLater);

            menu->addMenu(indicator);
        }
        const QStringList requests = iface.pairingRequests();
        if (!requests.isEmpty()) {
            menu->addSection(i18n("Pairing requests"));

            for (const auto &req : requests) {
                DeviceDbusInterface *dev = new DeviceDbusInterface(req, menu);
                auto pairMenu = menu->addMenu(dev->name());
                pairMenu->addAction(i18nc("Accept a pairing request", "Pair"), dev, &DeviceDbusInterface::acceptPairing);
                pairMenu->addAction(i18n("Reject"), dev, &DeviceDbusInterface::cancelPairing);
            }
        }
        // Add quit menu
#if defined Q_OS_MAC
        menu->addAction(i18n("Quit"), []() {
            auto message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kdeconnect.daemon"),
                                                          QStringLiteral("/MainApplication"),
                                                          QStringLiteral("org.qtproject.Qt.QCoreApplication"),
                                                          QStringLiteral("quit"));
            QDBusConnection::sessionBus().call(message, QDBus::NoBlock);
            qApp->quit();
        });
#elif defined Q_OS_WIN
        menu->addAction(QIcon::fromTheme(QStringLiteral("application-exit")), i18n("Quit"), []() {
            qApp->quit();
        });
#endif
    };

    QObject::connect(&iface, &DaemonDbusInterface::pairingRequestsChanged, &model, refreshMenu);
    QObject::connect(&model, &DevicesModel::rowsInserted, &model, refreshMenu);
    QObject::connect(&model, &DevicesModel::rowsRemoved, &model, refreshMenu);

    // Run icon to add icon path (if necessary)
    helper.iconPathHook();

    QSystemTrayIcon systray;
    helper.systrayIconHook(systray);
    systray.setVisible(true);
    systray.setToolTip(QStringLiteral("KDE Connect"));
    QObject::connect(&model, &DevicesModel::rowsChanged, &model, [&systray, &model]() {
        systray.setToolTip(i18np("%1 device connected", "%1 devices connected", model.rowCount()));
    });
    QObject::connect(&systray, &QSystemTrayIcon::activated, [](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            const QString kdeconnectAppExecutable = QStandardPaths::findExecutable(QStringLiteral("kdeconnect-app"), {QCoreApplication::applicationDirPath()});
            if (!kdeconnectAppExecutable.isEmpty()) {
                QProcess::startDetached(kdeconnectAppExecutable, {});
            }
        }
    });

    systray.setContextMenu(menu);

    refreshMenu();

    app.setQuitOnLastWindowClosed(false);

    return app.exec();
}
