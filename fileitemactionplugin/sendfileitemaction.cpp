/*
 * Copyright (C) 2011 Alejandro Fiestas Olivares <afiestas@kde.org>
 * Copyright (C) 2014 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "sendfileitemaction.h"

#include <QList>
#include <QMenu>
#include <QAction>
#include <QWidget>
#include <QVariantList>
#include <QUrl>
#include <QIcon>

#include <KPluginFactory>
#include <KPluginLoader>
#include <KLocalizedString>

#include <interfaces/devicesmodel.h>
#include <interfaces/dbusinterfaces.h>

K_PLUGIN_FACTORY(SendFileItemActionFactory, registerPlugin<SendFileItemAction>();)

Q_LOGGING_CATEGORY(KDECONNECT_FILEITEMACTION, "kdeconnect.fileitemaction")

SendFileItemAction::SendFileItemAction(QObject* parent, const QVariantList& ): KAbstractFileItemActionPlugin(parent)
{
}

QList<QAction*> SendFileItemAction::actions(const KFileItemListProperties& fileItemInfos, QWidget* parentWidget)
{
    QList<QAction*> actions;

    DaemonDbusInterface iface;
    if (!iface.isValid()) {
        return actions;
    }

    QDBusPendingReply<QStringList> reply = iface.devices(true, true);
    reply.waitForFinished();
    const QStringList devices = reply.value();
    for (const QString& id : devices) {
        DeviceDbusInterface deviceIface(id);
        if (!deviceIface.isValid()) {
            continue;
        }
        if (!deviceIface.hasPlugin(QStringLiteral("kdeconnect_share"))) {
            continue;
        }
        QAction* action = new QAction(QIcon::fromTheme(deviceIface.iconName()), deviceIface.name(), parentWidget);
        action->setProperty("id", id);
        action->setProperty("urls", QVariant::fromValue(fileItemInfos.urlList()));
        action->setProperty("parentWidget", QVariant::fromValue(parentWidget));
        connect(action, &QAction::triggered, this, &SendFileItemAction::sendFile);
        actions += action;
    }

    if (actions.count() > 1) {
        QAction* menuAction = new QAction(QIcon::fromTheme(QStringLiteral("preferences-system-network")), i18n("Send via KDE Connect"), parentWidget);
        QMenu* menu = new QMenu(parentWidget);
        menu->addActions(actions);
        menuAction->setMenu(menu);
        return QList<QAction*>() << menuAction;
    } else {
        if(actions.count() == 1) {
            actions.first()->setText(i18n("Send to '%1' via KDE Connect", actions.first()->text()));
        }
        return actions;
    }
}

void SendFileItemAction::sendFile()
{
    const QList<QUrl> urls = sender()->property("urls").value<QList<QUrl>>();
    QString id = sender()->property("id").toString();
    for (const QUrl& url : urls) {
        QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kdeconnect"), "/modules/kdeconnect/devices/"+id+"/share", QStringLiteral("org.kde.kdeconnect.device.share"), QStringLiteral("shareUrl"));
        msg.setArguments(QVariantList() << url.toString());
        QDBusConnection::sessionBus().call(msg);
    }
}

#include "sendfileitemaction.moc"
