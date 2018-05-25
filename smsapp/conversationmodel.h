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
#include <QLoggingCategory>

#include "interfaces/dbusinterfaces.h"

#include "interfaces/kdeconnectinterfaces_export.h"

Q_DECLARE_LOGGING_CATEGORY(KDECONNECT_SMS_CONVERSATION_MODEL)

class KDECONNECTINTERFACES_EXPORT ConversationModel
    : public QStandardItemModel
{
    Q_OBJECT
    Q_PROPERTY(QString threadId READ threadId WRITE setThreadId)
    Q_PROPERTY(QString deviceId READ deviceId WRITE setDeviceId)

public:
    ConversationModel(QObject* parent = nullptr);
    ~ConversationModel();

    enum Roles {
        FromMeRole = Qt::UserRole,
        DateRole,
    };

    QString threadId() const;
    void setThreadId(const QString &threadId);

    QString deviceId() const { return m_deviceId; }
    void setDeviceId(const QString &/*deviceId*/);

    Q_INVOKABLE void sendReplyToConversation(const QString& message);

private Q_SLOTS:
    void createRowFromMessage(const QVariantMap &msg, int pos);

private:
    DeviceConversationsDbusInterface* m_conversationsInterface;
    QString m_deviceId;
    QString m_threadId;
};

#endif // CONVERSATIONMODEL_H
