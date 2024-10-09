/**
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

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
    RequestConversationWorker(const qint64 &conversationID, int start, int end, ConversationsDbusInterface *interface);

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
    void conversationMessageRead(const QDBusVariant &msg);
    void finished();

private:
    qint64 conversationID;
    int start; // Start of range to request messages
    size_t howMany; // Number of messages being requested
    ConversationsDbusInterface *parent;

    QThread *m_thread;

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
    size_t replyForConversation(const QList<ConversationMessage> &conversation, size_t start, size_t howMany);
};
