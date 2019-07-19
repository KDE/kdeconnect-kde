/**
 * Copyright (C) 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 * Copyright (C) 2018 Simon Redman <simon@ergotech.com>
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

#include "conversationmodel.h"
#include <QLoggingCategory>

#include <KLocalizedString>

#include "interfaces/conversationmessage.h"
#include "smshelper.h"

Q_LOGGING_CATEGORY(KDECONNECT_SMS_CONVERSATION_MODEL, "kdeconnect.sms.conversation")

ConversationModel::ConversationModel(QObject* parent)
    : QStandardItemModel(parent)
    , m_conversationsInterface(nullptr)
{
    auto roles = roleNames();
    roles.insert(FromMeRole, "fromMe");
    roles.insert(DateRole, "date");
    roles.insert(SenderRole, "sender");
    roles.insert(AvatarRole, "avatar");
    setItemRoleNames(roles);
}

ConversationModel::~ConversationModel()
{
}

qint64 ConversationModel::threadId() const
{
    return m_threadId;
}

void ConversationModel::setThreadId(const qint64& threadId)
{
    if (m_threadId == threadId)
        return;

    m_threadId = threadId;
    clear();
    knownMessageIDs.clear();
    if (m_threadId != INVALID_THREAD_ID && !m_deviceId.isEmpty()) {
        requestMoreMessages();
    }
}

void ConversationModel::setDeviceId(const QString& deviceId)
{
    if (deviceId == m_deviceId)
        return;

    qCDebug(KDECONNECT_SMS_CONVERSATION_MODEL) << "setDeviceId" << "of" << this;
    if (m_conversationsInterface) {
        disconnect(m_conversationsInterface, SIGNAL(conversationUpdated(QDBusVariant)), this, SLOT(handleConversationUpdate(QDBusVariant)));
        delete m_conversationsInterface;
    }

    m_deviceId = deviceId;

    m_conversationsInterface = new DeviceConversationsDbusInterface(deviceId, this);
    connect(m_conversationsInterface, SIGNAL(conversationUpdated(QDBusVariant)), this, SLOT(handleConversationUpdate(QDBusVariant)));
}

void ConversationModel::sendReplyToConversation(const QString& message)
{
    //qCDebug(KDECONNECT_SMS_CONVERSATION_MODEL) << "Trying to send" << message << "to conversation with ID" << m_threadId;
    m_conversationsInterface->replyToConversation(m_threadId, message);
}

void ConversationModel::requestMoreMessages(const quint32& howMany)
{
    if (m_threadId == INVALID_THREAD_ID) {
        return;
    }
    const auto& numMessages = rowCount();
    m_conversationsInterface->requestConversation(m_threadId, numMessages, numMessages + howMany);
}

QString ConversationModel::getTitleForAddresses(const QList<ConversationAddress>& addresses) {
    return SmsHelper::getTitleForAddresses(addresses);
}

void ConversationModel::createRowFromMessage(const ConversationMessage& message, int pos)
{
    if (message.threadID() != m_threadId) {
        // Because of the asynchronous nature of the current implementation of this model, if the
        // user clicks quickly between threads or for some other reason a message comes when we're
        // not expecting it, we should not display it in the wrong place
        qCDebug(KDECONNECT_SMS_CONVERSATION_MODEL)
                << "Got a message for a thread" << message.threadID()
                << "but we are currently viewing" << m_threadId
                << "Discarding.";
        return;
    }

    if (knownMessageIDs.contains(message.uID())) {
        qCDebug(KDECONNECT_SMS_CONVERSATION_MODEL)
                << "Ignoring duplicate message with ID" << message.uID();
        return;
    }

    // TODO: Upgrade to support other kinds of media
    // Get the body that we should display
    QString displayBody = message.containsTextBody() ? message.body() : i18n("(Unsupported Message Type)");

    ConversationAddress sender = message.addresses().first();
    QString senderName = message.isMultitarget() ? SmsHelper::getTitleForAddresses({sender}) : QString();

    auto item = new QStandardItem;
    item->setText(displayBody);
    item->setData(message.type() == ConversationMessage::MessageTypeSent, FromMeRole);
    item->setData(message.date(), DateRole);
    item->setData(senderName, SenderRole);
    insertRow(pos, item);
    knownMessageIDs.insert(message.uID());
}

void ConversationModel::handleConversationUpdate(const QDBusVariant& msg)
{
    ConversationMessage message = ConversationMessage::fromDBus(msg);

    if (message.threadID() != m_threadId) {
        // If a conversation which we are not currently viewing was updated, discard the information
        qCDebug(KDECONNECT_SMS_CONVERSATION_MODEL)
                << "Saw update for thread" << message.threadID()
                << "but we are currently viewing" << m_threadId;
        return;
    }

    createRowFromMessage(message, 0);
}
