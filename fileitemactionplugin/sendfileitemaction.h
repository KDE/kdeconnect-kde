/*
 * SPDX-FileCopyrightText: 2011 Alejandro Fiestas Olivares <afiestas@kde.org>
 * SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SENDFILEITEMACTION_H
#define SENDFILEITEMACTION_H

#include <KAbstractFileItemActionPlugin>
#include <KFileItemListProperties>

class QAction;
class KFileItemListProperties;
class QWidget;

class SendFileItemAction : public KAbstractFileItemActionPlugin
{
    Q_OBJECT
public:
    SendFileItemAction(QObject *parent, const QVariantList &args); // TODO KF6 remove args
    QList<QAction *> actions(const KFileItemListProperties &fileItemInfos, QWidget *parentWidget) override;
};

#endif // SENDFILEITEMACTION_H
