/*
 * This file is part of KDE Telepathy Chat
 *
 * Copyright (C) 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 * Copyright (C) 2018 Simon Redman <simon@ergotech.com>
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

#include "conversationlistmodel.h"

#include <QLoggingCategory>
#include "interfaces/conversationmessage.h"
#include "interfaces/dbusinterfaces.h"

Q_LOGGING_CATEGORY(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL, "kdeconnect.sms.conversations_list")

ConversationListModel::ConversationListModel(QObject* parent)
    : QStandardItemModel(parent)
    , m_conversationsInterface(nullptr)
{
    qCDebug(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL) << "Constructing" << this;
    auto roles = roleNames();
    roles.insert(FromMeRole, "fromMe");
    roles.insert(AddressRole, "address");
    roles.insert(PersonUriRole, "personUri");
    roles.insert(ConversationIdRole, "conversationId");
    roles.insert(DateRole, "date");
    setItemRoleNames(roles);

    ConversationMessage::registerDbusType();
}

ConversationListModel::~ConversationListModel()
{
}

void ConversationListModel::setDeviceId(const QString& deviceId)
{
    if (deviceId == m_deviceId) {
        return;
    }

    qCDebug(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL) << "setDeviceId" << deviceId << "of" << this;

    if (m_conversationsInterface) {
        disconnect(m_conversationsInterface, SIGNAL(conversationCreated(QVariantMap)), this, SLOT(handleCreatedConversation(QVariantMap)));
        disconnect(m_conversationsInterface, SIGNAL(conversationUpdated(QVariantMap)), this, SLOT(handleConversationUpdated(QVariantMap)));
        delete m_conversationsInterface;
        m_conversationsInterface = nullptr;
    }

    m_deviceId = deviceId;

    // This method still gets called *with a valid deviceID* when the device is not connected while the component is setting up
    // Detect that case and don't do anything.
    DeviceDbusInterface device(deviceId);
    if (!(device.isValid() && device.isReachable())) {
        return;
    }

    m_conversationsInterface = new DeviceConversationsDbusInterface(deviceId, this);
    connect(m_conversationsInterface, SIGNAL(conversationCreated(QVariantMap)), this, SLOT(handleCreatedConversation(QVariantMap)));
    connect(m_conversationsInterface, SIGNAL(conversationUpdated(QVariantMap)), this, SLOT(handleConversationUpdated(QVariantMap)));
    prepareConversationsList();

    m_conversationsInterface->requestAllConversationThreads();
}

void ConversationListModel::prepareConversationsList()
{
    if (!m_conversationsInterface->isValid()) {
        qCWarning(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL) << "Tried to prepareConversationsList with an invalid interface!";
        return;
    }
    QDBusPendingReply<QVariantList> validThreadIDsReply = m_conversationsInterface->activeConversations();

    setWhenAvailable(validThreadIDsReply, [this](const QVariantList& convs) {
        clear(); // If we clear before we receive the reply, there might be a (several second) visual gap!
        for (const QVariant& headMessage : convs) {
            QDBusArgument data = headMessage.value<QDBusArgument>();
            QVariantMap message;
            data >> message;
            handleCreatedConversation(message);
        }
    }, this);
}

void ConversationListModel::handleCreatedConversation(const QVariantMap& msg)
{
    createRowFromMessage(msg);
}

void ConversationListModel::handleConversationUpdated(const QVariantMap& msg)
{
    createRowFromMessage(msg);
}

void ConversationListModel::printDBusError(const QDBusError& error)
{
    qCWarning(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL) << error;
}

QStandardItem * ConversationListModel::conversationForThreadId(qint32 threadId)
{
    for(int i=0, c=rowCount(); i<c; ++i) {
        auto it = item(i, 0);
        if (it->data(ConversationIdRole) == threadId)
            return it;
    }
    return nullptr;
}

void ConversationListModel::createRowFromMessage(const QVariantMap& msg)
{
    const ConversationMessage message(msg);
    if (message.type() == -1) {
        // The Android side currently hacks in -1 if something weird comes up
        // TODO: Remove this hack when MMS support is implemented
        return;
    }

    bool toadd = false;
    QStandardItem* item = conversationForThreadId(message.threadID());
    if (!item) {
        toadd = true;
        item = new QStandardItem();
        QScopedPointer<KPeople::PersonData> personData(lookupPersonByAddress(message.address()));
        if (personData) {
            item->setText(personData->name());
            item->setIcon(QIcon(personData->photo()));
            item->setData(personData->personUri(), PersonUriRole);
        } else {
            item->setData(QString(), PersonUriRole);
            item->setText(message.address());
        }
        item->setData(message.threadID(), ConversationIdRole);
    }

    // Update the message if the data is newer
    // This will be true if a conversation receives a new message, but false when the user
    // does something to trigger past conversation history loading
    bool oldDateExists;
    qint64 oldDate = item->data(DateRole).toLongLong(&oldDateExists);
    if (!oldDateExists || message.date() >= oldDate) {
        // If there was no old data or incoming data is newer, update the record
        item->setData(message.address(), AddressRole);
        item->setData(message.type() == ConversationMessage::MessageTypeSent, FromMeRole);
        item->setData(message.body(), Qt::ToolTipRole);
        item->setData(message.date(), DateRole);
    }

    if (toadd)
        appendRow(item);
}

KPeople::PersonData* ConversationListModel::lookupPersonByAddress(const QString& address)
{
    const QString& canonicalAddress = canonicalizePhoneNumber(address);
    int rowIndex = 0;
    for (rowIndex = 0; rowIndex < m_people.rowCount(); rowIndex++) {
        const QString& uri = m_people.get(rowIndex, KPeople::PersonsModel::PersonUriRole).toString();
        KPeople::PersonData* person = new KPeople::PersonData(uri);

        const QString& email = person->email();
        const QString& phoneNumber = canonicalizePhoneNumber(person->contactCustomProperty("phoneNumber").toString());

        // To decide if a phone number matches:
        // 1. Are they similar lengths? If two numbers are very different, probably one is junk data and should be ignored
        // 2. Is one a superset of the other? Phone number digits get more specific the further towards the end of the string,
        //    so if one phone number ends with the other, it is probably just a more-complete version of the same thing
        const QString& longerNumber = canonicalAddress.length() >= phoneNumber.length() ? canonicalAddress : phoneNumber;
        const QString& shorterNumber = canonicalAddress.length() < phoneNumber.length() ? canonicalAddress : phoneNumber;

        bool matchingPhoneNumber = longerNumber.endsWith(shorterNumber) && shorterNumber.length() * 2 >= longerNumber.length();

        if (address == email || matchingPhoneNumber) {
            qCDebug(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL) << "Matched" << address << "to" << person->name();
            return person;
        }

        delete person;
    }

    return nullptr;
}

QString ConversationListModel::canonicalizePhoneNumber(const QString& phoneNumber)
{
    QString toReturn(phoneNumber);
    toReturn = toReturn.remove(' ');
    toReturn = toReturn.remove('-');
    toReturn = toReturn.remove('(');
    toReturn = toReturn.remove(')');
    toReturn = toReturn.remove('+');
    toReturn = toReturn.remove(QRegularExpression("^0*")); // Strip leading zeroes
    return toReturn;
}
