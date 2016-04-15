/**
 * Copyright 2015 Holger Kaelberer <holger.k@elberer.de>
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

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>
#include <QtDebug>
#include <QLoggingCategory>

#include <kiconloader.h>
#include <kicontheme.h>

#include <core/device.h>
#include <core/kdeconnectplugin.h>

#include "notificationslistener.h"
#include "notificationsplugin.h"
#include "notification_debug.h"
#include "notificationsdbusinterface.h"
#include "notifyingapplication.h"

NotificationsListener::NotificationsListener(KdeConnectPlugin* aPlugin,
                                             NotificationsDbusInterface* aDbusInterface)
    : QDBusAbstractAdaptor(aPlugin),
      mPlugin(aPlugin),
      dbusInterface(aDbusInterface)
{
    qRegisterMetaTypeStreamOperators<NotifyingApplication>("NotifyingApplication");

    bool ret = QDBusConnection::sessionBus()
                .registerObject("/org/freedesktop/Notifications",
                                this,
                                QDBusConnection::ExportScriptableContents);
    if (!ret)
        qCWarning(KDECONNECT_PLUGIN_NOTIFICATION)
                << "Error registering notifications listener for device"
                << mPlugin->device()->name() << ":"
                << QDBusConnection::sessionBus().lastError();
    else
        qCDebug(KDECONNECT_PLUGIN_NOTIFICATION)
                << "Registered notifications listener for device"
                << mPlugin->device()->name();

    QDBusInterface iface("org.freedesktop.DBus", "/org/freedesktop/DBus",
                         "org.freedesktop.DBus");
    iface.call("AddMatch",
               "interface='org.freedesktop.Notifications',member='Notify',type='method_call',eavesdrop='true'");

    loadApplications();

    connect(mPlugin->config(), SIGNAL(configChanged()), this, SLOT(loadApplications()));
}

NotificationsListener::~NotificationsListener()
{
    qCDebug(KDECONNECT_PLUGIN_NOTIFICATION) << "Destroying NotificationsListener";
    QDBusInterface iface("org.freedesktop.DBus", "/org/freedesktop/DBus",
                         "org.freedesktop.DBus");
    QDBusMessage res = iface.call("RemoveMatch",
                                  "interface='org.freedesktop.Notifications',member='Notify',type='method_call',eavesdrop='true'");
    QDBusConnection::sessionBus().unregisterObject("/org/freedesktop/Notifications");
}

void NotificationsListener::loadApplications()
{
    applications.clear();
    QVariantList list = mPlugin->config()->getList("applications");
    for (const auto& a: list) {
        NotifyingApplication app = a.value<NotifyingApplication>();
        if (!applications.contains(app.name))
            applications.insert(app.name, app);
    }
    //qCDebug(KDECONNECT_PLUGIN_NOTIFICATION) << "Loaded" << applications.size() << " applications";
}

uint NotificationsListener::Notify(const QString &appName, uint replacesId,
                                   const QString &appIcon,
                                   const QString &summary, const QString &body,
                                   const QStringList &actions,
                                   const QVariantMap &hints, int timeout)
{
    static int id = 0;
    Q_UNUSED(actions);

    //qCDebug(KDECONNECT_PLUGIN_NOTIFICATION) << "Got notification appName=" << appName << "replacesId=" << replacesId << "appIcon=" << appIcon << "summary=" << summary << "body=" << body << "actions=" << actions << "hints=" << hints << "timeout=" << timeout;

    // skip our own notifications
    if (appName == QLatin1String("KDE Connect"))
        return 0;

    NotifyingApplication app;
    if (!applications.contains(appName)) {
        // new application -> add to config
        app.name = appName;
        app.icon = appIcon;
        app.active = true;
        app.blacklistExpression = QRegularExpression();
        applications.insert(app.name, app);
        // update config:
        QVariantList list;
        for (const auto& a: applications.values())
            list << QVariant::fromValue<NotifyingApplication>(a);
        mPlugin->config()->setList("applications", list);
        //qCDebug(KDECONNECT_PLUGIN_NOTIFICATION) << "Added new application to config:" << app;
    } else
        app = applications.value(appName);

    if (!app.active)
        return 0;

    if (timeout > 0 && mPlugin->config()->get("generalPersistent", false))
        return 0;

    int urgency = -1;
    if (hints.contains("urgency")) {
        bool ok;
        urgency = hints["urgency"].toInt(&ok);
        if (!ok)
            urgency = -1;
    }
    if (urgency > -1 && urgency < mPlugin->config()->get<int>("generalUrgency", 0))
        return 0;

    QString ticker = summary;
    if (!body.isEmpty() && mPlugin->config()->get("generalIncludeBody", true))
        ticker += QLatin1String(": ") + body;

    if (app.blacklistExpression.isValid() &&
            !app.blacklistExpression.pattern().isEmpty() &&
            app.blacklistExpression.match(ticker).hasMatch())
        return 0;

    //qCDebug(KDECONNECT_PLUGIN_NOTIFICATION) << "Sending notification from" << appName << ":" <<ticker << "; appIcon=" << appIcon;
    NetworkPackage np(PACKAGE_TYPE_NOTIFICATION);
    np.set("id", QString::number(replacesId > 0 ? replacesId : ++id));
    np.set("appName", appName);
    np.set("ticker", ticker);
    np.set("isClearable", timeout == 0);  // KNotifications are persistent if
                                          // timeout == 0, for other notifications
                                          // clearability is pointless

    if (!appIcon.isEmpty() && mPlugin->config()->get("generalSynchronizeIcons", true)) {
        int size = KIconLoader::SizeEnormous;  // use big size to allow for good
                                               // quality on High-DPI mobile devices
        QString iconPath = KIconLoader::global()->iconPath(appIcon, -size, true);
        if (!iconPath.isEmpty()) {
            if (!iconPath.endsWith(QLatin1String(".png")) &&
                    KIconLoader::global()->theme()->name() != QLatin1String("hicolor")) {
                // try falling back to hicolor theme:
                KIconTheme hicolor(QStringLiteral("hicolor"));
                if (hicolor.isValid()) {
                    iconPath = hicolor.iconPath(appIcon + ".png", size, KIconLoader::MatchBest);
                    //qCDebug(KDECONNECT_PLUGIN_NOTIFICATION) << "Found non-png icon in default theme trying fallback to hicolor:" << iconPath;
                }
            }
            if (iconPath.endsWith(QLatin1String(".png"))) {
                //qCDebug(KDECONNECT_PLUGIN_NOTIFICATION) << "Appending icon " << iconPath;
                QSharedPointer<QIODevice> iconFile(new QFile(iconPath));
                np.setPayload(iconFile, iconFile->size());
            }
        }
    }

    mPlugin->sendPackage(np);

    return (replacesId > 0 ? replacesId : id);
}
