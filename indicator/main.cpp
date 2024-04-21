/*
 * SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QApplication>
#include <QPointer>
#include <QProcess>
#include <QThread>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

#ifdef Q_OS_WIN
#include <QSystemTrayIcon>
#else
#include <KStatusNotifierItem>
#endif

#include <KAboutData>
#include <KCMultiDialog>
#include <KColorSchemeManager>
#include <KDBusService>
#include <KLocalizedString>

#include "deviceindicator.h"
#include "interfaces/dbusinterfaces.h"
#include "interfaces/devicesmodel.h"
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

    QIcon::setFallbackThemeName(QStringLiteral("breeze"));

    QApplication app(argc, argv);
    KAboutData about(QStringLiteral("kdeconnect-indicator"),
                     i18n("KDE Connect Indicator"),
                     QStringLiteral(KDECONNECT_VERSION_STRING),
                     i18n("KDE Connect Indicator tool"),
                     KAboutLicense::GPL,
                     i18n("(C) 2016 Aleix Pol Gonzalez"));
    KAboutData::setApplicationData(about);

#ifdef Q_OS_WIN
    KColorSchemeManager manager;
    QApplication::setStyle(QStringLiteral("breeze"));
    IndicatorHelper helper(QUrl::fromLocalFile(qApp->applicationDirPath()));
#else
    IndicatorHelper helper;
#endif

    helper.preInit();

    // Run Daemon initialization step
    // When run from macOS app bundle, D-Bus call should be later than kdeconnectd and D-Bus daemon
    QProcess kdeconnectd;
    if (helper.daemonHook(kdeconnectd)) {
        return -1;
    }

    KDBusService dbusService(KDBusService::Unique);

    // Trigger loading the KIconLoader plugin
    about.setProgramLogo(QIcon(QStringLiteral(":/icons/kdeconnect/kdeconnect.svg")));

    DevicesModel model;
    model.setDisplayFilter(DevicesModel::Reachable | DevicesModel::Paired);
    QMenu *menu = new QMenu;

    QPointer<KCMultiDialog> dialog;

    DaemonDbusInterface iface;

    auto refreshMenu = [&iface, &model, &menu, &dialog]() {
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
        for (int i = 0, count = model.rowCount(); i < count; ++i) {
            DeviceDbusInterface *device = model.getDevice(i);
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

#ifdef Q_OS_WIN
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
#else
    KStatusNotifierItem systray;
    helper.systrayIconHook(systray);
    systray.setToolTip(QStringLiteral("kdeconnect"), QStringLiteral("KDE Connect"), QStringLiteral("KDE Connect"));
    systray.setCategory(KStatusNotifierItem::Communications);
    systray.setStatus(KStatusNotifierItem::Passive);
    systray.setStandardActionsEnabled(false);
    QObject::connect(&model, &DevicesModel::rowsChanged, &model, [&systray, &model]() {
        const auto count = model.rowCount();
#ifndef Q_OS_MACOS // On MacOS, setting status to Active disables color theme syncing of the menu icon
        systray.setStatus(count == 0 ? KStatusNotifierItem::Passive : KStatusNotifierItem::Active);
#endif
        systray.setToolTip(QStringLiteral("kdeconnect"), QStringLiteral("KDE Connect"), i18np("%1 device connected", "%1 devices connected", count));
    });

    systray.setContextMenu(menu);
#endif

    refreshMenu();

    app.setQuitOnLastWindowClosed(false);

    // Finish init
    helper.postInit();

    return app.exec();
}
