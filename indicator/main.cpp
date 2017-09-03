/*
 * Copyright 2016 Aleix Pol Gonzalez <aleixpol@kde.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QCryptographicHash>
#include <QIODevice>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QApplication>
#include <QTextStream>
#include <QProcess>

#ifdef QSYSTRAY
#include <QSystemTrayIcon>
#else
#include <KStatusNotifierItem>
#endif

#include <KDBusService>
#include <KAboutData>
#include <KCMultiDialog>
#include <KLocalizedString>

#include "interfaces/devicesmodel.h"
#include "interfaces/notificationsmodel.h"
#include "interfaces/dbusinterfaces.h"
#include "kdeconnect-version.h"
#include "deviceindicator.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    KAboutData about("kdeconnect-indicator",
                     i18n("KDE Connect Indicator"),
                     QStringLiteral(KDECONNECT_VERSION_STRING),
                     i18n("KDE Connect Indicator tool"),
                     KAboutLicense::GPL,
                     i18n("(C) 2016 Aleix Pol Gonzalez"));
    KAboutData::setApplicationData(about);

#ifdef Q_OS_WIN
    QProcess::startDetached("kdeconnectd.exe");
#endif

    KDBusService dbusService(KDBusService::Unique);

    DevicesModel model;
    model.setDisplayFilter(DevicesModel::Reachable | DevicesModel::Paired);
    QMenu* menu = new QMenu;

    DaemonDbusInterface iface;
    auto refreshMenu = [&iface, &model, &menu]() {
        menu->clear();
        auto configure = menu->addAction(i18n("Configure..."));
        QObject::connect(configure, &QAction::triggered, configure, [](){
            KCMultiDialog* dialog = new KCMultiDialog;
            dialog->addModule("kcm_kdeconnect");
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
        });
        for (int i=0, count = model.rowCount(); i<count; ++i) {
            DeviceDbusInterface* device = model.getDevice(i);
            auto indicator = new DeviceIndicator(device);
            QObject::connect(device, &DeviceDbusInterface::destroyed, indicator, &QObject::deleteLater);

            menu->addMenu(indicator);
        }
        const QStringList requests = iface.pairingRequests();
        if (!requests.isEmpty()) {
            menu->addSection(i18n("Pairing requests"));

            for(const auto& req: requests) {
                DeviceDbusInterface* dev = new DeviceDbusInterface(req, menu);
                auto pairMenu = menu->addMenu(dev->name());
                pairMenu->addAction(i18n("Pair"), dev, &DeviceDbusInterface::acceptPairing);
                pairMenu->addAction(i18n("Reject"), dev, &DeviceDbusInterface::rejectPairing);
            }
        }
    };

    QObject::connect(&iface, &DaemonDbusInterface::pairingRequestsChangedProxy, &model, refreshMenu);
    QObject::connect(&model, &DevicesModel::rowsInserted, &model, refreshMenu);
    QObject::connect(&model, &DevicesModel::rowsRemoved, &model, refreshMenu);

#ifdef QSYSTRAY
    QSystemTrayIcon systray;
    systray.setIcon(QIcon::fromTheme("kdeconnect"));
    systray.setVisible(true);
    systray.setToolTip("KDE Connect");
    QObject::connect(&model, &DevicesModel::rowsChanged, &model, [&systray, &model]() {
        systray.setToolTip(i18np("%1 device connected", "%1 devices connected", model.rowCount()));
    });

    systray.setContextMenu(menu);
#else
    KStatusNotifierItem systray;
    systray.setIconByName(QStringLiteral("kdeconnect"));
    systray.setToolTip(QStringLiteral("kdeconnect"), "KDE Connect", "KDE Connect");
    systray.setCategory(KStatusNotifierItem::Communications);
    systray.setStatus(KStatusNotifierItem::Passive);
    systray.setStandardActionsEnabled(false);
    QObject::connect(&model, &DevicesModel::rowsChanged, &model, [&systray, &model]() {
        const auto count = model.rowCount();
        systray.setStatus(count == 0 ? KStatusNotifierItem::Passive : KStatusNotifierItem::Active);
        systray.setToolTip(QStringLiteral("kdeconnect"), QStringLiteral("KDE Connect"), i18np("%1 device connected", "%1 devices connected", count));
    });

    systray.setContextMenu(menu);
#endif

    refreshMenu();

    app.setQuitOnLastWindowClosed(false);

    return app.exec();
}
