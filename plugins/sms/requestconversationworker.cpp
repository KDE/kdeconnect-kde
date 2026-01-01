/**
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "requestconversationworker.h"

#include "conversationsdbusinterface.h"

#include <QObject>

RequestConversationWorker::RequestConversationWorker(const qint64 &conversationID, int start, int end, ConversationsDbusInterface *interface)
    : // QObject(interface)
    conversationID(conversationID)
    , start(start)
    , parent(interface)
    , m_thread(new QThread)
{
    Q_ASSERT(end >= start && "Not allowed to have a negative-length range");
    howMany = end - start;

    this->moveToThread(m_thread);
    connect(m_thread, &QThread::started, this, &RequestConversationWorker::handleRequestConversation);
    connect(m_thread, &QThread::finished, m_thread, &QObject::deleteLater);
    connect(this, &RequestConversationWorker::finished, m_thread, &QThread::quit);
    connect(this, &RequestConversationWorker::finished, this, &QObject::deleteLater);
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
        // In this case, the cache wasn't able to satisfy the request fully. Get more.

        size_t numRemaining = howMany - numHandled;
        parent->updateConversation(conversationID);
        messagesList = parent->getConversation(conversationID);
        // ConversationsDbusInterface::updateConversation blocks until it sees new messages in the requested conversation
        replyForConversation(messagesList, start + numHandled, numRemaining);
    } else {
        // The cache was able to fully satisfy the request but we need to check that it isn't running dry

        size_t numCachedMessages = messagesList.count();
        size_t requestEnd = start + numHandled;
        size_t numRemainingMessages = numCachedMessages - requestEnd;
        double percentRemaining = ((double)numRemainingMessages / numCachedMessages) * 100;

        if (percentRemaining < CACHE_LOW_WATER_MARK_PERCENT || numRemainingMessages < MIN_NUMBER_TO_REQUEST) {
            parent->updateConversation(conversationID);
        }
    }

    Q_EMIT finished();
}

size_t RequestConversationWorker::replyForConversation(const QList<ConversationMessage> &conversation, size_t start, size_t howMany)
{
    // Messages are sorted in ascending order of keys, meaning the front of the list has the oldest
    // messages (smallest timestamp number)
    // Therefore, return the end of the list first (most recent messages)
    size_t i = 0;
    for (auto it = conversation.crbegin() + start; it < conversation.crend(); ++it) {
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

#include "moc_requestconversationworker.cpp"
