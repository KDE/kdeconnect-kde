/**
 * SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef CONVERSATIONMODEL_H
#define CONVERSATIONMODEL_H

#include <QQueue>
#include <QSet>
#include <QStandardItemModel>
#include <qtmetamacros.h>

#include "dbusinterfaces/dbusinterfaces.h"
#include "models/conversationmessage.h"
#include "thumbnailsprovider.h"

#define INVALID_THREAD_ID -1

class ConversationModel : public QStandardItemModel
{
    Q_OBJECT
    Q_PROPERTY(qint64 threadId READ threadId WRITE setThreadId)
    Q_PROPERTY(QString deviceId READ deviceId WRITE setDeviceId NOTIFY deviceIdChanged)
    Q_PROPERTY(QList<ConversationAddress> addressList READ addressList WRITE setAddressList)

public:
    ConversationModel(QObject *parent = nullptr);
    ~ConversationModel() override;

    enum Roles {
        FromMeRole = Qt::UserRole,
        SenderRole, // The sender of the message. Undefined if this is an outgoing message
        DateRole,
        AvatarRole, // URI to the avatar of the sender of the message. Undefined if outgoing.
        AttachmentsRole, // The list of attachments. Undefined if there is no attachment in a message
    };

    Q_ENUM(Roles)

    qint64 threadId() const;
    void setThreadId(const qint64 &threadId);

    QString deviceId() const
    {
        return m_deviceId;
    }
    void setDeviceId(const QString & /*deviceId*/);

    QList<ConversationAddress> addressList() const
    {
        return m_addressList;
    }
    void setAddressList(const QList<ConversationAddress> &addressList);

    Q_INVOKABLE bool sendReplyToConversation(const QString &textMessage, QList<QUrl> attachmentUrls);
    Q_INVOKABLE bool startNewConversation(const QString &textMessage, const QList<ConversationAddress> &addressList, QList<QUrl> attachmentUrls);

    Q_INVOKABLE QString getCharCountInfo(const QString &message) const;

    Q_INVOKABLE void requestAttachmentPath(const qint64 &partID, const QString &UniqueIdentifier);

    enum class RequestMessageRole {
        FetchMore, // Corresponds to standardItemModel fetchMore, always tries to fetch at least one item
        UiPrefetch, // Ui preloading more items when getting close to the top of the list, best effort
        UiTimer, // The safety timer, re-fetches the pending items. Matters when a phone reconnects or similar situations
    };
    Q_ENUM(RequestMessageRole);

    Q_INVOKABLE void requestMessages(enum ConversationModel::RequestMessageRole role);

    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

Q_SIGNALS:
    void gotNewMessage();
    void filePathReceived(QString filePath, QString fileName);
    void deviceIdChanged(const QString &value);

private Q_SLOTS:
    void handleConversationUpdate(const QDBusVariant &message);
    void handleConversationCreated(const QDBusVariant &message);

private:
    void createRowFromMessage(const ConversationMessage &message);

    DeviceConversationsDbusInterface *m_conversationsInterface;
    std::optional<ThumbnailsProvider *> m_thumbnailsProvider;
    QString m_deviceId;
    qint64 m_threadId = INVALID_THREAD_ID;
    QList<ConversationAddress> m_addressList;
    QSet<qint32> knownMessageIDs; // Set of known Message uIDs

    std::optional<qint64> lastRequestedMessageIndex = std::nullopt;
};

#endif // CONVERSATIONMODEL_H
