/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

class ConversationsDbusInterface
    : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.conversations")

public:
    explicit ConversationsDbusInterface(KdeConnectPlugin* plugin);
    ~ConversationsDbusInterface() override;

    void addMessage(const ConversationMessage &message);
    void removeMessage(const QString& internalId);

public Q_SLOTS:
    /**
     * Return a list of the threadID for all valid conversations
     */
    QStringList activeConversations();

    void requestConversation(const QString &conversationID, int start, int end);

    /**
     * Send a new message to this conversation
     */
    void replyToConversation(const QString& conversationID, const QString& message);

    /**
     * Send the request to the Telephony plugin to update the list of conversation threads
     */
    void requestAllConversationThreads();

Q_SIGNALS:
    Q_SCRIPTABLE void conversationCreated(const QString& threadID);
    Q_SCRIPTABLE void conversationRemoved(const QString& threadID);
    Q_SCRIPTABLE void conversationUpdated(const QString& threadID);
    Q_SCRIPTABLE void conversationMessageReceived(const QVariantMap & msg, int pos) const;

private /*methods*/:
    QString newId(); //Generates successive identifitiers to use as public ids

private /*attributes*/:
    const Device* m_device;
    KdeConnectPlugin* m_plugin;

    /**
     * Mapping of threadID to the messages which make up that thread
     *
     * The messages are stored as a QMap of the timestamp to the actual message object so that
     * we can use .values() to get a sorted list of messages from least- to most-recent
     */
    QHash<QString, QMap<qint64, ConversationMessage>> m_conversations;

    /**
     * Mapping of threadID to the set of uIDs known in the corresponding conversation
     */
    QHash<QString, QSet<qint32>> m_known_messages;

    int m_lastId;

    TelephonyDbusInterface m_telephonyInterface;
};

#endif // CONVERSATIONSDBUSINTERFACE_H
