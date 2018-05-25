/*
 * This file is part of KDE Telepathy Chat
 *
 * Copyright (C) 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "conversationmodel.h"
#include <QLoggingCategory>
#include "interfaces/conversationmessage.h"

Q_LOGGING_CATEGORY(KDECONNECT_SMS_CONVERSATION_MODEL, "kdeconnect.sms.conversation")

ConversationModel::ConversationModel(QObject* parent)
    : QStandardItemModel(parent)
    , m_conversationsInterface(nullptr)
{
    auto roles = roleNames();
    roles.insert(FromMeRole, "fromMe");
    roles.insert(DateRole, "date");
    setItemRoleNames(roles);
}

ConversationModel::~ConversationModel()
{
}

QString ConversationModel::threadId() const
{
    return m_threadId;
}

void ConversationModel::setThreadId(const QString &threadId)
{
    if (m_threadId == threadId)
        return;

    m_threadId = threadId;
    clear();
    if (!threadId.isEmpty()) {
        m_conversationsInterface->requestConversation(threadId, 0, 10);
    }
}

void ConversationModel::setDeviceId(const QString& deviceId)
{
    if (deviceId == m_deviceId)
        return;

    qCDebug(KDECONNECT_SMS_CONVERSATION_MODEL) << "setDeviceId" << "of" << this;
    if (m_conversationsInterface) delete m_conversationsInterface;

    m_deviceId = deviceId;

    m_conversationsInterface = new DeviceConversationsDbusInterface(deviceId, this);
    connect(m_conversationsInterface, SIGNAL(conversationMessageReceived(QVariantMap, int)), this, SLOT(createRowFromMessage(QVariantMap, int)));
}

void ConversationModel::sendReplyToConversation(const QString& message)
{
    qCDebug(KDECONNECT_SMS_CONVERSATION_MODEL) << "Trying to send" << message << "to conversation with ID" << m_threadId;
    m_conversationsInterface->replyToConversation(m_threadId, message);
}

void ConversationModel::createRowFromMessage(const QVariantMap& msg, int pos)
{
    const ConversationMessage message(msg);
    auto item = new QStandardItem;
    item->setText(message.body());
    item->setData(message.type() == ConversationMessage::MessageTypeSent, FromMeRole);
    item->setData(message.date(), DateRole);
    insertRow(pos, item);
}
