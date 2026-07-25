/*
 * SPDX-FileCopyrightText: 2011 Alejandro Fiestas Olivares <afiestas@kde.org>
 * SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2026 Kai Uwe Broulik <kde@broulik.de>
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

#include "deviceactionjob.h"

K_PLUGIN_CLASS_WITH_JSON(SendFileItemAction, "kdeconnectsendfile.json")

SendFileItemAction::SendFileItemAction(QObject *parent, const QVariantList &)
    : KAbstractFileItemActionPlugin(parent)
{
}

QList<QAction *> SendFileItemAction::actions(const KFileItemListProperties &fileItemInfos, QWidget *parentWidget)
{
    // We have to return an action right away.
    // Return a placeholder that we then asynchronously populate.
    auto *action = new QAction(QIcon::fromTheme(QStringLiteral("kdeconnect")), i18n("Send via KDE Connect"), parentWidget);
    action->setVisible(false);

    auto *job = new DeviceActionJob(fileItemInfos.urlList(), parentWidget);
    connect(job, &DeviceActionJob::finished, parentWidget, [action, parentWidget](const QList<QAction *> &actions) {
        if (actions.count() > 1) {
            QMenu *menu = new QMenu(parentWidget);
            menu->addActions(actions);
            action->setMenu(menu);
            action->setVisible(true);
        } else if (actions.count() == 1) {
            auto *firstAction = actions.first();
            action->setText(i18n("Send to '%1' via KDE Connect", firstAction->text()));
            connect(action, &QAction::triggered, firstAction, &QAction::trigger);
            action->setVisible(true);
        }
    });
    job->start(parentWidget);

    return {action};
}

#include "moc_sendfileitemaction.cpp"
#include "sendfileitemaction.moc"
