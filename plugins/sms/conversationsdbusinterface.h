/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QDBusAbstractAdaptor>
#include <QDir>
#include <QHash>
#include <QList>
#include <QMap>
#include <QPointer>
#include <QString>
#include <QStringList>

#include "dbusinterfaces/dbusinterfaces.h"
#include "models/conversationmessage.h"

class KdeConnectPlugin;
class Device;

Q_DECLARE_LOGGING_CATEGORY(KDECONNECT_CONVERSATIONS)

// There is some amount of overhead and delay to making a request, so make sure to request at least a few
#define MIN_NUMBER_TO_REQUEST 25
// Some low-water mark after which we want to fill the cache
#define CACHE_LOW_WATER_MARK_PERCENT 10

class ConversationsDbusInterface : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.conversations")

public:
    explicit ConversationsDbusInterface(KdeConnectPlugin *plugin);
    ~ConversationsDbusInterface() override;

    void addMessages(const QList<ConversationMessage> &messages);
    void removeMessage(const QString &internalId);

    /**
     * Return a shallow copy of the requested conversation
     */
    QList<ConversationMessage> getConversation(const qint64 &conversationID) const;

    /**
     * Get some new messages for the requested conversation from the remote device
     * Requests a quantity of new messages equal to the current number of messages in the conversation
     */
    void updateConversation(const qint64 &conversationID);

    /**
     * Gets the path of the successfully downloaded attachment file and send
     * update to the conversationModel
     */
    void attachmentDownloaded(const QString &filePath, const QString &fileName);

public Q_SLOTS:
    /**
     * Return a list of the first message in every conversation
     *
     * Note that the return value is a list of QVariants, which in turn have a value of
     * QVariantMap created from each message
     */
    QVariantList activeConversations();

    /**
     * Request the specified range of the specified conversation
     *
     * Emits conversationUpdated for every message in the requested range
     *
     * If the conversation does not have enough messages to fill the request,
     * this method may return fewer messages
     */
    void requestConversation(const qint64 &conversationID, int start, int end);

    /**
     * Send a new message to this conversation
     */
    void replyToConversation(const qint64 &conversationID, const QString &message, const QVariantList &attachmentUrls);

    /**
     * Send a new message to the contact having no previous conversation with
     */
    void sendWithoutConversation(const QVariantList &addressList, const QString &message, const QVariantList &attachmentUrls);

    /**
     * Send the request to the Telephony plugin to update the list of conversation threads
     */
    void requestAllConversationThreads();

    /**
     * Send the request to SMS plugin to fetch original attachment file path
     */
    void requestAttachmentFile(const qint64 &partID, const QString &uniqueIdentifier);

Q_SIGNALS:

    /**
     * This signal is never emitted, but if it's not here then qdbuscpp2xml in Qt6
     * will not generate all the signals that use QDBusVariant in the output XML
     */
    Q_SCRIPTABLE void veryHackyWorkaround(const QVariant &);

    /**
     * Emitted whenever a conversation with no cached messages is added, either because the cache
     * is being populated or because a new conversation has been created
     */
    Q_SCRIPTABLE void conversationCreated(const QDBusVariant &msg);

    /**
     * Emitted whenever a conversation is being deleted
     */
    Q_SCRIPTABLE void conversationRemoved(const qint64 &conversationID);

    /**
     * Emitted whenever a message is added to a conversation and it is the newest message in the
     * conversation
     */
    Q_SCRIPTABLE void conversationUpdated(const QDBusVariant &msg);

    /**
     * Emitted whenever we have handled a response from the phone indicating the total number of
     * (locally-known) messages in the given conversation
     */
    Q_SCRIPTABLE void conversationLoaded(qint64 conversationID, quint64 messageCount);

    /**
     * Emitted whenever we have successfully download a requested attachment file from the phone
     */
    Q_SCRIPTABLE void attachmentReceived(QString filePath, QString fileName);

private: // methods
    QString newId(); // Generates successive identifiers to use as public ids

private: // attributes
    const QString m_device;

    /**
     * Mapping of threadID to the messages which make up that thread
     *
     * The messages are stored as a QMap of the timestamp to the actual message object so that
     * we can use .values() to get a sorted list of messages from least- to most-recent
     */
    QHash<qint64, QMap<qint64, ConversationMessage>> m_conversations;

    /**
     * Mapping of threadID to the set of uIDs known in the corresponding conversation
     */
    QHash<qint64, QSet<qint32>> m_known_messages;

    /*
     * Keep a map of all interfaces ever constructed
     * Because of how Qt's Dbus is designed, we are unable to immediately delete the interface once
     * the device has disconnected. We save the list of existing interfaces and delete them only after
     * we have replaced them (in ConversationsDbusInterface's constructor)
     * See the comment in ~NotificationsPlugin() for more information
     */
    static QMap<QString, ConversationsDbusInterface *> liveConversationInterfaces;

    int m_lastId;

    SmsDbusInterface m_smsInterface;

    QSet<qint64> conversationsWaitingForMessages;
    QMutex waitingForMessagesLock;
    QWaitCondition waitingForMessages;
};
