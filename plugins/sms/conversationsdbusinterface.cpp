/**
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "conversationsdbusinterface.h"
#include "interfaces/conversationmessage.h"
#include "interfaces/dbusinterfaces.h"

#include "requestconversationworker.h"

#include <QDBusConnection>

#include <core/device.h>
#include <core/kdeconnectplugin.h>

#include "kdeconnect_conversations_debug.h"

QMap<QString, ConversationsDbusInterface *> ConversationsDbusInterface::liveConversationInterfaces;

ConversationsDbusInterface::ConversationsDbusInterface(KdeConnectPlugin *plugin)
    : QDBusAbstractAdaptor(const_cast<Device *>(plugin->device()))
    , m_device(plugin->device()->id())
    , m_lastId(0)
    , m_smsInterface(m_device)
{
    ConversationMessage::registerDbusType();

    // Check for an existing interface for the same device
    // If there is already an interface for this device, we can safely delete is since we have just replaced it
    const auto &oldInterfaceItr = ConversationsDbusInterface::liveConversationInterfaces.find(m_device);
    if (oldInterfaceItr != ConversationsDbusInterface::liveConversationInterfaces.end()) {
        ConversationsDbusInterface *oldInterface = oldInterfaceItr.value();
        oldInterface->deleteLater();
        ConversationsDbusInterface::liveConversationInterfaces.erase(oldInterfaceItr);
    }

    ConversationsDbusInterface::liveConversationInterfaces[m_device] = this;
}

ConversationsDbusInterface::~ConversationsDbusInterface()
{
    // Wake all threads which were waiting for a reply from this interface
    // This might result in some noise on dbus, but it's better than leaking a bunch of resources!
    waitingForMessagesLock.lock();
    conversationsWaitingForMessages.clear();
    waitingForMessages.wakeAll();
    waitingForMessagesLock.unlock();

    // Erase this interface from the list of known interfaces
    const auto myIterator = ConversationsDbusInterface::liveConversationInterfaces.find(m_device);
    ConversationsDbusInterface::liveConversationInterfaces.erase(myIterator);
}

QVariantList ConversationsDbusInterface::activeConversations()
{
    QList<QVariant> toReturn;
    toReturn.reserve(m_conversations.size());

    for (auto it = m_conversations.cbegin(); it != m_conversations.cend(); ++it) {
        const auto &conversation = it.value().values();
        if (conversation.isEmpty()) {
            // This should really never happen because we create a conversation at the same time
            // as adding a message, but better safe than sorry
            qCWarning(KDECONNECT_CONVERSATIONS) << "Conversation with ID" << it.key() << "is unexpectedly empty";
            break;
        }
        const QVariant &message = QVariant::fromValue<ConversationMessage>(*conversation.crbegin());
        toReturn.append(message);
    }

    return toReturn;
}

void ConversationsDbusInterface::requestConversation(const qint64 &conversationID, int start, int end)
{
    if (start < 0 || end < 0) {
        qCWarning(KDECONNECT_CONVERSATIONS) << "requestConversation"
                                            << "Start and end must be >= 0";
        return;
    }

    if (end - start < 0) {
        qCWarning(KDECONNECT_CONVERSATIONS) << "requestConversation"
                                            << "Start must be before end";
        return;
    }

    RequestConversationWorker *worker = new RequestConversationWorker(conversationID, start, end, this);
    connect(worker, &RequestConversationWorker::conversationMessageRead, this, &ConversationsDbusInterface::conversationUpdated, Qt::QueuedConnection);
    worker->work();
}

void ConversationsDbusInterface::requestAttachmentFile(const qint64 &partID, const QString &uniqueIdentifier)
{
    m_smsInterface.getAttachment(partID, uniqueIdentifier);
}

void ConversationsDbusInterface::addMessages(const QList<ConversationMessage> &messages)
{
    QSet<qint64> updatedConversationIDs;

    for (const auto &message : messages) {
        const qint32 &threadId = message.threadID();

        // We might discover that there are no new messages in this conversation, thus calling it
        // "updated" might turn out to be a bit misleading
        // However, we need to report it as updated regardless, for the case where we have already
        // cached every message of the conversation but we have received a request for more, otherwise
        // we will never respond to that request
        updatedConversationIDs.insert(message.threadID());

        if (m_known_messages[threadId].contains(message.uID())) {
            // This message has already been processed. Don't do anything.
            continue;
        }

        // Store the Message in the list corresponding to its thread
        bool newConversation = !m_conversations.contains(threadId);
        const auto &threadPosition = m_conversations[threadId].insert(message.date(), message);
        m_known_messages[threadId].insert(message.uID());

        // If this message was inserted at the end of the list, it is the latest message in the conversation
        const bool latestMessage = std::distance(threadPosition, m_conversations[threadId].end()) == 1;

        // Tell the world about what just happened
        if (newConversation) {
            Q_EMIT conversationCreated(QDBusVariant(QVariant::fromValue(message)));
        } else if (latestMessage) {
            Q_EMIT conversationUpdated(QDBusVariant(QVariant::fromValue(message)));
        }
    }

    // It feels bad to go through the set of updated conversations again,
    // but also there are not many times that updatedConversationIDs will be more than one
    for (qint64 conversationID : updatedConversationIDs) {
        quint64 numMessages = m_known_messages[conversationID].size();
        Q_EMIT conversationLoaded(conversationID, numMessages);
    }

    waitingForMessagesLock.lock();
    // Remove the waiting flag for all conversations which we just processed
    conversationsWaitingForMessages.subtract(updatedConversationIDs);
    waitingForMessages.wakeAll();
    waitingForMessagesLock.unlock();
}

void ConversationsDbusInterface::removeMessage(const QString &internalId)
{
    // TODO: Delete the specified message from our internal structures
    Q_UNUSED(internalId);
}

QList<ConversationMessage> ConversationsDbusInterface::getConversation(const qint64 &conversationID) const
{
    return m_conversations.value(conversationID).values();
}

void ConversationsDbusInterface::updateConversation(const qint64 &conversationID)
{
    waitingForMessagesLock.lock();
    if (conversationsWaitingForMessages.contains(conversationID)) {
        // This conversation is already being waited on, don't allow more than one thread to wait at a time
        qCDebug(KDECONNECT_CONVERSATIONS) << "Not allowing two threads to wait for conversationID" << conversationID;
        waitingForMessagesLock.unlock();
        return;
    }
    qCDebug(KDECONNECT_CONVERSATIONS) << "Requesting conversation with ID" << conversationID << "from remote";
    conversationsWaitingForMessages.insert(conversationID);

    // Request a window of messages
    qint64 rangeStartTimestamp;
    qint64 numberToRequest;
    if (m_conversations.contains(conversationID) && m_conversations[conversationID].count() > 0) {
        rangeStartTimestamp = m_conversations[conversationID].first().date(); // Request starting from the oldest-available message
        numberToRequest = m_conversations[conversationID].count(); // Request an increasing number of messages by requesting more equal to the amount we have
    } else {
        rangeStartTimestamp = -1; // Value < 0 indicates to return the newest messages
        numberToRequest = MIN_NUMBER_TO_REQUEST; // Start off with a small batch
    }
    if (numberToRequest < MIN_NUMBER_TO_REQUEST) {
        numberToRequest = MIN_NUMBER_TO_REQUEST;
    }
    m_smsInterface.requestConversation(conversationID, rangeStartTimestamp, numberToRequest);

    while (conversationsWaitingForMessages.contains(conversationID)) {
        waitingForMessages.wait(&waitingForMessagesLock);
    }
    waitingForMessagesLock.unlock();
}

void ConversationsDbusInterface::replyToConversation(const qint64 &conversationID, const QString &message, const QVariantList &attachmentUrls)
{
    const auto messagesList = m_conversations[conversationID];
    if (messagesList.isEmpty()) {
        qCWarning(KDECONNECT_CONVERSATIONS) << "Got a conversationID for a conversation with no messages!";
        return;
    }

    const QList<ConversationAddress> &addressList = messagesList.first().addresses();
    QVariantList addresses;

    for (const auto &address : addressList) {
        addresses << QVariant::fromValue(address);
    }

    m_smsInterface.sendSms(addresses, message, attachmentUrls, messagesList.first().subID());
}

void ConversationsDbusInterface::sendWithoutConversation(const QVariantList &addresses, const QString &message, const QVariantList &attachmentUrls)
{
    m_smsInterface.sendSms(addresses, message, attachmentUrls);
}

void ConversationsDbusInterface::requestAllConversationThreads()
{
    // Prepare the list of conversations by requesting the first in every thread
    m_smsInterface.requestAllConversations();
}

QString ConversationsDbusInterface::newId()
{
    return QString::number(++m_lastId);
}

void ConversationsDbusInterface::attachmentDownloaded(const QString &filePath, const QString &fileName)
{
    Q_EMIT attachmentReceived(filePath, fileName);
}

#include "moc_conversationsdbusinterface.cpp"
