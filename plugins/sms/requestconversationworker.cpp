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

#include "requestconversationworker.h"

#include "conversationsdbusinterface.h"

#include <QObject>

RequestConversationWorker::RequestConversationWorker(const qint64& conversationID, int start, int end, ConversationsDbusInterface* interface) :
        //QObject(interface)
        conversationID(conversationID)
        , start(start)
        , end(end)
        , parent(interface)
        , m_thread(new QThread)
{
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

    // TODO: Reply with all messages we currently have available, even if we don't have enough to completely fill the request
    // In case the remote takes awhile to respond, we should go ahead and do anything we can from the cache

    if (messagesList.length() <= end) {
        // If we don't have enough messages in cache, go get some more
        // TODO: Make Android interface capable of requesting small window of messages
        parent->updateConversation(conversationID);
        messagesList = parent->getConversation(conversationID);
    }

    // Messages are sorted in ascending order of keys, meaning the front of the list has the oldest
    // messages (smallest timestamp number)
    // Therefore, return the end of the list first (most recent messages)
    int i = start;
    for(auto it = messagesList.crbegin() + start; it != messagesList.crend(); ++it) {
        Q_EMIT conversationMessageRead(it->toVariant());
        i++;
        if (i >= end) {
            break;
        }
    }

    Q_EMIT finished();
}

void RequestConversationWorker::work()
{
    m_thread->start();
}
