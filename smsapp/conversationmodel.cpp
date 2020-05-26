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

#include <KLocalizedString>

#include "interfaces/conversationmessage.h"
#include "smshelper.h"

#include "sms_conversation_debug.h"

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
        disconnect(m_conversationsInterface, SIGNAL(conversationLoaded(qint64, quint64)), this, SLOT(handleConversationLoaded(qint64, quint64)));
        disconnect(m_conversationsInterface, SIGNAL(conversationCreated(QDBusVariant)), this, SLOT(handleConversationCreated(QDBusVariant)));
        delete m_conversationsInterface;
    }

    m_deviceId = deviceId;

    m_conversationsInterface = new DeviceConversationsDbusInterface(deviceId, this);
    connect(m_conversationsInterface, SIGNAL(conversationUpdated(QDBusVariant)), this, SLOT(handleConversationUpdate(QDBusVariant)));
    connect(m_conversationsInterface, SIGNAL(conversationLoaded(qint64, quint64)), this, SLOT(handleConversationLoaded(qint64, quint64)));
    connect(m_conversationsInterface, SIGNAL(conversationCreated(QDBusVariant)), this, SLOT(handleConversationCreated(QDBusVariant)));

    connect(this, SIGNAL(sendMessageWithoutConversation(QDBusVariant, QString)), m_conversationsInterface, SLOT(sendWithoutConversation(QDBusVariant, QString)));
}

void ConversationModel::setAddressList(const QList<ConversationAddress>& addressList) {
    m_addressList = addressList;
}

void ConversationModel::sendReplyToConversation(const QString& message)
{
    //qCDebug(KDECONNECT_SMS_CONVERSATION_MODEL) << "Trying to send" << message << "to conversation with ID" << m_threadId;
    m_conversationsInterface->replyToConversation(m_threadId, message);
}

void ConversationModel::startNewConversation(const QString& message, const QList<ConversationAddress>& addressList)
{
    QVariant addresses;
    addresses.setValue(addressList);
    Q_EMIT sendMessageWithoutConversation(QDBusVariant(addresses), message);
}

void ConversationModel::requestMoreMessages(const quint32& howMany)
{
    if (m_threadId == INVALID_THREAD_ID) {
        return;
    }
    const auto& numMessages = rowCount();
    m_conversationsInterface->requestConversation(m_threadId, numMessages, numMessages + howMany);
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
    QString senderName = message.isIncoming() ? SmsHelper::getTitleForAddresses({sender}) : QString();

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

void ConversationModel::handleConversationCreated(const QDBusVariant& msg)
{
    ConversationMessage message = ConversationMessage::fromDBus(msg);

    if (m_threadId == INVALID_THREAD_ID && SmsHelper::isPhoneNumberMatch(m_addressList[0].address(), message.addresses().first().address()) && !message.isMultitarget()) {
        m_threadId = message.threadID();
        createRowFromMessage(message, 0);
    }
}

void ConversationModel::handleConversationLoaded(qint64 threadID, quint64 numMessages)
{
    Q_UNUSED(numMessages)
    if (threadID != m_threadId) {
        return;
    }
    // If we get this flag, it means that the phone will not be responding with any more messages
    // so we should not be showing a loading indicator
    Q_EMIT loadingFinished();
}

QString ConversationModel::getCharCountInfo(const QString& message) const
{
    SmsCharCount count = SmsHelper::getCharCount(message);

    if (count.messages > 1) {
        // Show remaining char count and message count
        return QString::number(count.remaining) + QLatin1Char('/') + QString::number(count.messages);
    }
    if (count.messages == 1 && count.remaining < 10) {
        // Show only remaining char count
        return QString::number(count.remaining);
    }
    else {
        // Do not show anything
        return QString();
    }
}
