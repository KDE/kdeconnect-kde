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

#ifndef SENDFILEITEMACTION_H
#define SENDFILEITEMACTION_H

#include <KFileItemListProperties>
#include <KAbstractFileItemActionPlugin>
#include <QLoggingCategory>

class QAction;
class KFileItemListProperties;
class QWidget;

Q_DECLARE_LOGGING_CATEGORY(KDECONNECT_FILEITEMACTION)
class SendFileItemAction : public KAbstractFileItemActionPlugin
{
Q_OBJECT
public:
    SendFileItemAction(QObject* parent, const QVariantList& args);
    QList< QAction* > actions(const KFileItemListProperties& fileItemInfos, QWidget* parentWidget) override;

private Q_SLOTS:
    void sendFile();

};

#endif // SENDFILEITEMACTION_H
