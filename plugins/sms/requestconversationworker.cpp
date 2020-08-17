/**
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "requestconversationworker.h"

#include "conversationsdbusinterface.h"

#include <QObject>

RequestConversationWorker::RequestConversationWorker(const qint64& conversationID, int start, int end, ConversationsDbusInterface* interface) :
        //QObject(interface)
        conversationID(conversationID)
        , start(start)
        , parent(interface)
        , m_thread(new QThread)
{
    Q_ASSERT(end >= start && "Not allowed to have a negative-length range");
    howMany = end - start;

    this->moveToThread(m_thread);
    connect(m_thread, &QThread::started,
            this, &RequestConversationWorker::handleRequestConversation);
    connect(m_thread, &QThread::finished,
            m_thread, &QObject::deleteLater);
    connect(this, &RequestConversationWorker::finished,
            m_thread, &QThread::quit);
    connect(this, &RequestConversationWorker::finished,
            this, &QObject::deleteLater);
}

void RequestConversationWorker::handleRequestConversation()
{
    auto messagesList = parent->getConversation(conversationID);

    if (messagesList.isEmpty()) {
        // Since there are no messages in the conversation, it's likely that it is a junk ID, but go ahead anyway
        qCWarning(KDECONNECT_CONVERSATIONS) << "Got a conversationID for a conversation with no messages!" << conversationID;
    }

    // In case the remote takes awhile to respond, we should go ahead and do anything we can from the cache
    size_t numHandled = replyForConversation(messagesList, start, howMany);

    if (numHandled < howMany) {
        size_t numRemaining = howMany - numHandled;
        // If we don't have enough messages in cache, go get some more
        // TODO: Make Android interface capable of requesting small window of messages
        parent->updateConversation(conversationID);
        messagesList = parent->getConversation(conversationID);
        //ConversationsDbusInterface::getConversation blocks until it sees new messages in the requested conversation
        replyForConversation(messagesList, start + numHandled, numRemaining);
    }

    Q_EMIT finished();
}

size_t RequestConversationWorker::replyForConversation(const QList<ConversationMessage>& conversation, int start, size_t howMany) {
    Q_ASSERT(start >= 0);
    // Messages are sorted in ascending order of keys, meaning the front of the list has the oldest
    // messages (smallest timestamp number)
    // Therefore, return the end of the list first (most recent messages)
    size_t i = 0;
    for(auto it = conversation.crbegin() + start; it != conversation.crend(); ++it) {
        if (i >= howMany) {
            break;
        }
        Q_EMIT conversationMessageRead(QDBusVariant(QVariant::fromValue(*it)));
        i++;
    }

    return i;
}

void RequestConversationWorker::work()
{
    m_thread->start();
}
