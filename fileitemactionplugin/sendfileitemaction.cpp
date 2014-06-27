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
#include <interfaces/devicesmodel.h>
#include <interfaces/dbusinterfaces.h>

#include <QList>
#include <QMenu>
#include <QAction>
#include <QWidget>
#include <QMessageBox>
#include <QVariantList>

#include <KIcon>
#include <KPluginFactory>
#include <KPluginLoader>

#include <KDebug>
#include <KProcess>
#include <KLocalizedString>

K_PLUGIN_FACTORY(SendFileItemActionFactory, registerPlugin<SendFileItemAction>();)
K_EXPORT_PLUGIN(SendFileItemActionFactory("SendFileItemAction", "kdeconnect-filetiemaction"))

SendFileItemAction::SendFileItemAction(QObject* parent, const QVariantList& ): KFileItemActionPlugin(parent)
{
}

QList<QAction*> SendFileItemAction::actions(const KFileItemListProperties& fileItemInfos, QWidget* parentWidget) const
{
    DevicesModel m;

    QList<QAction*> actions;

    for(int i = 0; i<m.rowCount(); ++i) {
        QModelIndex idx = m.index(i);
        DeviceDbusInterface* dev = m.getDevice(idx);
        if(dev->isReachable() && dev->isPaired()) {
            QAction* action = new QAction(QIcon::fromTheme(dev->iconName()), dev->name(), parentWidget);
            action->setProperty("id", idx.data(DevicesModel::IdModelRole));
            action->setProperty("urls", QVariant::fromValue(fileItemInfos.urlList()));
            action->setProperty("parentWidget", QVariant::fromValue(parentWidget));
            connect(action, SIGNAL(triggered(bool)), this, SLOT(sendFile()));
            actions += action;
        }
    }

    if (actions.count() > 1) {
        QAction *menuAction = new QAction(QIcon::fromTheme("preferences-system-network"), i18n("Send via KDE Connect"), parentWidget);
        QMenu *menu = new QMenu();
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
    QList<QUrl> urls = sender()->property("urls").value<KUrl::List>();
    foreach(const QUrl& url, urls) {
        QDBusMessage msg = QDBusMessage::createMethodCall("org.kde.kdeconnect", "/modules/kdeconnect/devices/"+sender()->property("id").toString()+"/share", "org.kde.kdeconnect.device.share", "shareUrl");
        msg.setArguments(QVariantList() << url.toString());

        QDBusConnection::sessionBus().call(msg);
    }
}
