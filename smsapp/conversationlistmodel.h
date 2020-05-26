/**
 * Copyright (C) 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 * Copyright (C) 2018 Simon Redman <simon@ergotech.com>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CONVERSATIONLISTMODEL_H
#define CONVERSATIONLISTMODEL_H

#include <QStandardItemModel>

#include "interfaces/conversationmessage.h"
#include "interfaces/dbusinterfaces.h"

class ConversationListModel
    : public QStandardItemModel
{
    Q_OBJECT
    Q_PROPERTY(QString deviceId READ deviceId WRITE setDeviceId NOTIFY deviceIdChanged)

public:
    ConversationListModel(QObject* parent = nullptr);
    ~ConversationListModel();

    enum Roles {
        /* Roles which apply while working as a single message */
        FromMeRole = Qt::UserRole,
        SenderRole,      // The sender of the message. Undefined if this is an outgoing message
        DateRole,        // The date of this message
        /* Roles which apply while working as the head of a conversation */
        AddressesRole,      // The Addresses involved in the conversation
        ConversationIdRole, // The ThreadID of the conversation
        MultitargetRole,    // Indicate that this conversation is multitarget
    };
    Q_ENUM(Roles)

    QString deviceId() const { return m_deviceId; }
    void setDeviceId(const QString &/*deviceId*/);

    Q_SCRIPTABLE void refresh();

    /**
     * This method creates conversation with an arbitrary address
     */
    Q_INVOKABLE void createConversationForAddress(const QString& address);

public Q_SLOTS:
    void handleCreatedConversation(const QDBusVariant& msg);
    void handleConversationUpdated(const QDBusVariant& msg);
    void createRowFromMessage(const ConversationMessage& message);
    void printDBusError(const QDBusError& error);
    void displayContacts();

Q_SIGNALS:
    void deviceIdChanged();

private:
    /**
     * Get all conversations currently known by the conversationsInterface, if any
     */
    void prepareConversationsList();

    QStandardItem* conversationForThreadId(qint32 threadId);
    QStandardItem* getConversationForAddress(const QString& address);

    DeviceConversationsDbusInterface* m_conversationsInterface;
    QString m_deviceId;
};

#endif // CONVERSATIONLISTMODEL_H
