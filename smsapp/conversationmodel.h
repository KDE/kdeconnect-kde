/*
 * This file is part of KDE Telepathy Chat
 *
 * Copyright (C) 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef CONVERSATIONMODEL_H
#define CONVERSATIONMODEL_H

#include <QStandardItemModel>

class ConversationModel : public QStandardItemModel
{
    Q_OBJECT
    Q_PROPERTY(QString threadId READ threadId WRITE setThreadId)
    Q_PROPERTY(QString deviceId READ deviceId WRITE setDeviceId)

public:
    ConversationModel(QObject* parent = nullptr);

    enum Roles { FromMeRole = Qt::UserRole };

    QString threadId() const;
    void setThreadId(const QString &threadId);

    QString deviceId() const { return {}; }
    void setDeviceId(const QString &/*deviceId*/) {}
};

#endif // CONVERSATIONMODEL_H
