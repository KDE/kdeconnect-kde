/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
 * Copyright 2018 Simon Redman <simon@ergotech.com>
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

#ifndef CONVERSATIONSDBUSINTERFACE_H
#define CONVERSATIONSDBUSINTERFACE_H

#include <QDBusAbstractAdaptor>
#include <QHash>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QPointer>

#include "interfaces/conversationmessage.h"
#include "interfaces/dbusinterfaces.h"

class KdeConnectPlugin;
class Device;

Q_DECLARE_LOGGING_CATEGORY(KDECONNECT_CONVERSATIONS)

class ConversationsDbusInterface
    : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.conversations")

public:
    explicit ConversationsDbusInterface(KdeConnectPlugin* plugin);
    ~ConversationsDbusInterface() override;

    void addMessages(const QList<ConversationMessage> &messages);
    void removeMessage(const QString& internalId);

    /**
     * Return a shallow copy of the requested conversation
     */
    QList<ConversationMessage> getConversation(const qint64& conversationID) const;

    /**
     * Get all of the messages in the requested conversation from the remote device
     * TODO: Make interface capable of requesting smaller window of messages
     */
    void updateConversation(const qint64& conversationID);

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
    void replyToConversation(const qint64& conversationID, const QString& message);

    /**
     * Send a new message to the contact having no previous coversation with
     */
    void sendWithoutConversation(const QDBusVariant& addressList, const QString& message);

    /**
     * Send the request to the Telephony plugin to update the list of conversation threads
     */
    void requestAllConversationThreads();

Q_SIGNALS:
    /**
     * Emitted whenever a conversation with no cached messages is added, either because the cache
     * is being populated or because a new conversation has been created
     */
    Q_SCRIPTABLE void conversationCreated(const QDBusVariant& msg);

    /**
     * Emitted whenever a conversation is being deleted
     */
    Q_SCRIPTABLE void conversationRemoved(const qint64& conversationID);

    /**
     * Emitted whenever a message is added to a conversation and it is the newest message in the
     * conversation
     */
    Q_SCRIPTABLE void conversationUpdated(const QDBusVariant& msg);

    /**
     * Emitted whenever we have handled a response from the phone indicating the total number of
     * (locally-known) messages in the given conversation
     */
    Q_SCRIPTABLE void conversationLoaded(qint64 conversationID, quint64 messageCount);

private /*methods*/:
    QString newId(); //Generates successive identifiers to use as public ids

private /*attributes*/:
    const QString m_device;
    KdeConnectPlugin* m_plugin;

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
    static QMap<QString, ConversationsDbusInterface*> liveConversationInterfaces;

    int m_lastId;

    SmsDbusInterface m_smsInterface;

    QSet<qint64> conversationsWaitingForMessages;
    QMutex waitingForMessagesLock;
    QWaitCondition waitingForMessages;
};

#endif // CONVERSATIONSDBUSINTERFACE_H
