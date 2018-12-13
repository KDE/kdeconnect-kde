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
    void handleRequestConversation();
    void work();

Q_SIGNALS:
    void conversationMessageRead(const QVariantMap& msg) const;
    void finished();

private:
    qint64 conversationID;
    int start;
    int end;
    ConversationsDbusInterface* parent;

    QThread* m_thread;
};

#endif // REQUESTCONVERSATIONWORKER_H
