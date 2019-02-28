/**
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef REQUESTCONVERSATIONWORKER_H
#define REQUESTCONVERSATIONWORKER_H

#include "conversationsdbusinterface.h"

#include <QObject>
#include <QThread>

/**
 * In case we need to wait for more messages to be downloaded from Android,
 * Do the actual work of a requestConversation call in a separate thread
 *
 * This class is the worker for that thread
 */
class RequestConversationWorker : public QObject
{
    Q_OBJECT

public:
    RequestConversationWorker(const qint64& conversationID, int start, int end, ConversationsDbusInterface* interface);


public Q_SLOTS:
    /**
     * Main body of this worker
     *
     * Reply to a request for messages and, if needed, wait for the remote to reply with more
     *
     * Emits conversationMessageRead for every message handled
     */
    void handleRequestConversation();
    void work();

Q_SIGNALS:
    void conversationMessageRead(const QVariantMap& msg);
    void finished();

private:
    qint64 conversationID;
    int start; // Start of range to request messages
    size_t howMany; // Number of messages being requested
    ConversationsDbusInterface* parent;

    QThread* m_thread;

    /**
     * Reply with all messages we currently have available in the requested range
     *
     * If the range specified by start and howMany is not in the range of messages in the conversation,
     * reply with only as many messages as we have available in that range
     *
     * @param conversation Conversation to send messages from
     * @param start Start of requested range, 0-indexed, inclusive
     * @param howMany Maximum number of messages to return
     * $return Number of messages processed
     */
    size_t replyForConversation(const QList<ConversationMessage>& conversation, int start, size_t howMany);
};

#endif // REQUESTCONVERSATIONWORKER_H
