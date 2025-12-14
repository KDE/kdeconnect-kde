/*
 * SPDX-FileCopyrightText: 2011 Alejandro Fiestas Olivares <afiestas@kde.org>
 * SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "sendfileitemaction.h"

#include <QAction>
#include <QIcon>
#include <QList>
#include <QMenu>
#include <QUrl>
#include <QVariantList>
#include <QWidget>

#include <KLocalizedString>
#include <KPluginFactory>

#include "dbusinterfaces/dbusinterfaces.h"
#include "models/devicesmodel.h"

#include <dbushelper.h>

#include "kdeconnect_fileitemaction_debug.h"

K_PLUGIN_CLASS_WITH_JSON(SendFileItemAction, "kdeconnectsendfile.json")

SendFileItemAction::SendFileItemAction(QObject *parent, const QVariantList &)
    : KAbstractFileItemActionPlugin(parent)
{
}

QList<QAction *> SendFileItemAction::actions(const KFileItemListProperties &fileItemInfos, QWidget *parentWidget)
{
    QList<QAction *> actions;

    DaemonDbusInterface iface;
    if (!iface.isValid()) {
        return actions;
    }

    QDBusPendingReply<QStringList> reply = iface.devices(true, true);
    reply.waitForFinished();
    const QStringList devices = reply.value();
    for (const QString &id : devices) {
        DeviceDbusInterface deviceIface(id);
        if (!deviceIface.isValid()) {
            continue;
        }
        if (!deviceIface.hasPlugin(QStringLiteral("kdeconnect_share"))) {
            continue;
        }
        QAction *action = new QAction(QIcon::fromTheme(deviceIface.iconName()), deviceIface.name(), parentWidget);
        action->setProperty("id", id);
        const QList<QUrl> urls = fileItemInfos.urlList();
        connect(action, &QAction::triggered, this, [id, urls]() {
            for (const QUrl &url : urls) {
                QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kdeconnect"),
                                                                  QLatin1String("/modules/kdeconnect/devices/%1/share").arg(id),
                                                                  QStringLiteral("org.kde.kdeconnect.device.share"),
                                                                  QStringLiteral("shareUrl"));
                msg.setArguments(QVariantList{url.toString()});
                QDBusConnection::sessionBus().asyncCall(msg);
            }
        });
        actions += action;
    }

    if (actions.count() > 1) {
        QAction *menuAction = new QAction(QIcon::fromTheme(QStringLiteral("kdeconnect")), i18n("Send via KDE Connect"), parentWidget);
        QMenu *menu = new QMenu(parentWidget);
        menu->addActions(actions);
        menuAction->setMenu(menu);
        return QList<QAction *>() << menuAction;
    } else {
        if (actions.count() == 1) {
            actions.first()->setText(i18n("Send to '%1' via KDE Connect", actions.first()->text()));
        }
        return actions;
    }
}

#include "moc_sendfileitemaction.cpp"
#include "sendfileitemaction.moc"
