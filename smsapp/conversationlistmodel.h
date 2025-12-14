/**
 * SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef CONVERSATIONLISTMODEL_H
#define CONVERSATIONLISTMODEL_H

#include <QStandardItemModel>

#include "dbusinterfaces/dbusinterfaces.h"
#include "models/conversationmessage.h"

class ConversationListModel : public QStandardItemModel
{
    Q_OBJECT
    Q_PROPERTY(QString deviceId READ deviceId WRITE setDeviceId NOTIFY deviceIdChanged)

public:
    ConversationListModel(QObject *parent = nullptr);
    ~ConversationListModel() override;

    enum Roles {
        /* Roles which apply while working as a single message */
        FromMeRole = Qt::UserRole,
        SenderRole, // The sender of the message. Undefined if this is an outgoing message
        DateRole, // The date of this message
        /* Roles which apply while working as the head of a conversation */
        AddressesRole, // The Addresses involved in the conversation
        ConversationIdRole, // The ThreadID of the conversation
        MultitargetRole, // Indicate that this conversation is multitarget
        AttachmentPreview, // A thumbnail of the attachment of the message, if any
    };
    Q_ENUM(Roles)

    QString deviceId() const
    {
        return m_deviceId;
    }
    void setDeviceId(const QString & /*deviceId*/);

    Q_SCRIPTABLE void refresh();

    /**
     * This method creates conversation with an arbitrary address
     */
    Q_INVOKABLE void createConversationForAddress(const QString &address);

public Q_SLOTS:
    void handleCreatedConversation(const QDBusVariant &msg);
    void handleConversationUpdated(const QDBusVariant &msg);
    void createRowFromMessage(const ConversationMessage &message);
    void printDBusError(const QDBusError &error);
    void displayContacts();

Q_SIGNALS:
    void deviceIdChanged();

private:
    /**
     * Get all conversations currently known by the conversationsInterface, if any
     */
    void prepareConversationsList();

    QStandardItem *conversationForThreadId(qint32 threadId);
    QStandardItem *getConversationForAddress(const QString &address);

    DeviceConversationsDbusInterface *m_conversationsInterface;
    QString m_deviceId;
};

#endif // CONVERSATIONLISTMODEL_H
