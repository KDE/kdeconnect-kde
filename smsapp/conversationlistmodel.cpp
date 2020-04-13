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

#include "conversationlistmodel.h"

#include <QString>
#include <QLoggingCategory>
#include <QPainter>

#include <KLocalizedString>

#include "interfaces/conversationmessage.h"
#include "interfaces/dbusinterfaces.h"
#include "smshelper.h"

Q_LOGGING_CATEGORY(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL, "kdeconnect.sms.conversations_list")

#define INVALID_THREAD_ID -1
#define INVALID_DATE -1

ConversationListModel::ConversationListModel(QObject* parent)
    : QStandardItemModel(parent)
    , m_conversationsInterface(nullptr)
{
    //qCDebug(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL) << "Constructing" << this;
    auto roles = roleNames();
    roles.insert(FromMeRole, "fromMe");
    roles.insert(SenderRole, "sender");
    roles.insert(DateRole, "date");
    roles.insert(AddressesRole, "addresses");
    roles.insert(ConversationIdRole, "conversationId");
    roles.insert(MultitargetRole, "isMultitarget");
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

    if (deviceId.isEmpty()) {
        return;
    }

    qCDebug(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL) << "setDeviceId" << deviceId << "of" << this;

    if (m_conversationsInterface) {
        disconnect(m_conversationsInterface, SIGNAL(conversationCreated(QDBusVariant)), this, SLOT(handleCreatedConversation(QDBusVariant)));
        disconnect(m_conversationsInterface, SIGNAL(conversationUpdated(QDBusVariant)), this, SLOT(handleConversationUpdated(QDBusVariant)));
        delete m_conversationsInterface;
        m_conversationsInterface = nullptr;
    }

    // This method still gets called *with a valid deviceID* when the device is not connected while the component is setting up
    // Detect that case and don't do anything.
    DeviceDbusInterface device(deviceId);
    if (!(device.isValid() && device.isReachable())) {
        return;
    }

    m_deviceId = deviceId;
    Q_EMIT deviceIdChanged();

    m_conversationsInterface = new DeviceConversationsDbusInterface(deviceId, this);
    connect(m_conversationsInterface, SIGNAL(conversationCreated(QDBusVariant)), this, SLOT(handleCreatedConversation(QDBusVariant)));
    connect(m_conversationsInterface, SIGNAL(conversationUpdated(QDBusVariant)), this, SLOT(handleConversationUpdated(QDBusVariant)));

    refresh();
}

void ConversationListModel::refresh()
{
    if (m_deviceId.isEmpty()) {
        qWarning() << "refreshing null device";
        return;
    }

    prepareConversationsList();
    m_conversationsInterface->requestAllConversationThreads();
}

void ConversationListModel::prepareConversationsList()
{
    if (!m_conversationsInterface->isValid()) {
        qCWarning(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL) << "Tried to prepareConversationsList with an invalid interface!";
        return;
    }
    const QDBusPendingReply<QVariantList> validThreadIDsReply = m_conversationsInterface->activeConversations();

    setWhenAvailable(validThreadIDsReply, [this](const QVariantList& convs) {
        clear(); // If we clear before we receive the reply, there might be a (several second) visual gap!
        for (const QVariant& headMessage : convs) {
            const QDBusArgument data = headMessage.value<QDBusArgument>();
            ConversationMessage message;
            data >> message;
            createRowFromMessage(message);
        }
        displayContacts();
    }, this);
}

void ConversationListModel::handleCreatedConversation(const QDBusVariant& msg)
{
    const ConversationMessage message = ConversationMessage::fromDBus(msg);
    createRowFromMessage(message);
}

void ConversationListModel::handleConversationUpdated(const QDBusVariant& msg)
{
    const ConversationMessage message = ConversationMessage::fromDBus(msg);
    createRowFromMessage(message);
}

void ConversationListModel::printDBusError(const QDBusError& error)
{
    qCWarning(KDECONNECT_SMS_CONVERSATIONS_LIST_MODEL) << error;
}

QStandardItem* ConversationListModel::conversationForThreadId(qint32 threadId)
{
    for(int i=0, c=rowCount(); i<c; ++i) {
        auto it = item(i, 0);
        if (it->data(ConversationIdRole) == threadId)
            return it;
    }
    return nullptr;
}

QStandardItem* ConversationListModel::getConversationForAddress(const QString& address)
{
    for(int i = 0; i < rowCount(); ++i) {
        const auto& it = item(i, 0);
        if (!it->data(MultitargetRole).toBool()) {
            if (SmsHelper::isPhoneNumberMatch(it->data(SenderRole).toString(), address)) {
                return it;
            }
        }
    }
    return nullptr;
}

void ConversationListModel::createRowFromMessage(const ConversationMessage& message)
{
    if (message.type() == -1) {
        // The Android side currently hacks in -1 if something weird comes up
        // TODO: Remove this hack when MMS support is implemented
        return;
    }

    /** The address of everyone involved in this conversation, which we should not display (check if they are known contacts first) */
    const QList<ConversationAddress> rawAddresses = message.addresses();
    if (rawAddresses.isEmpty()) {
        qWarning() << "no addresses!" << message.body();
        return;
    }

    bool toadd = false;
    QStandardItem* item = conversationForThreadId(message.threadID());
    //Check if we have a contact with which to associate this message, needed if there is no conversation with the contact and we received a message from them
    if (!item && !message.isMultitarget()) {

            item = getConversationForAddress(rawAddresses[0].address());
            if (item) {
                item->setData(message.threadID(), ConversationIdRole);
            }
        }

    if (!item) {
        toadd = true;
        item = new QStandardItem();

        const QString displayNames = SmsHelper::getTitleForAddresses(rawAddresses);
        const QIcon displayIcon = SmsHelper::getIconForAddresses(rawAddresses);

        item->setText(displayNames);
        item->setIcon(displayIcon);
        item->setData(message.threadID(), ConversationIdRole);
        item->setData(rawAddresses[0].address(), SenderRole);
    }

    // TODO: Upgrade to support other kinds of media
    // Get the body that we should display
    QString displayBody = message.containsTextBody() ? message.body() : i18n("(Unsupported Message Type)");

    // Prepend the sender's name
    if (message.isOutgoing()) {
        displayBody = i18n("You: %1", displayBody);
    } else {
        // If the message is incoming, the sender is the first Address
        const QString senderAddress = item->data(SenderRole).toString();
        const auto sender = SmsHelper::lookupPersonByAddress(senderAddress);
        const QString senderName = sender == nullptr? senderAddress : SmsHelper::lookupPersonByAddress(senderAddress)->name();
        displayBody = i18n("%1: %2", senderName, displayBody);
    }

    // Update the message if the data is newer
    // This will be true if a conversation receives a new message, but false when the user
    // does something to trigger past conversation history loading
    bool oldDateExists;
    const qint64 oldDate = item->data(DateRole).toLongLong(&oldDateExists);
    if (!oldDateExists || message.date() >= oldDate) {
        // If there was no old data or incoming data is newer, update the record
        item->setData(QVariant::fromValue(message.addresses()), AddressesRole);
        item->setData(message.isOutgoing(), FromMeRole);
        item->setData(displayBody, Qt::ToolTipRole);
        item->setData(message.date(), DateRole);
        item->setData(message.isMultitarget(), MultitargetRole);
    }

    if (toadd)
        appendRow(item);
}

void ConversationListModel::displayContacts()
{
    const QList<QSharedPointer<KPeople::PersonData>> personDataList = SmsHelper::getAllPersons();

    for(const auto& person : personDataList) {
        const QVariantList allPhoneNumbers = person->contactCustomProperty(QStringLiteral("all-phoneNumber")).toList();

        for (const QVariant& rawPhoneNumber : allPhoneNumbers) {
            //check for any duplicate phoneNumber and eliminate it
            if (!getConversationForAddress(rawPhoneNumber.toString())) {
                QStandardItem* item = new QStandardItem();
                item->setText(person->name());
                item->setIcon(person->photo());

                QList<ConversationAddress> addresses;
                addresses.append(ConversationAddress(rawPhoneNumber.toString()));
                item->setData(QVariant::fromValue(addresses), AddressesRole);

                const QString displayBody = i18n("%1", rawPhoneNumber.toString());
                item->setData(displayBody, Qt::ToolTipRole);
                item->setData(false, MultitargetRole);
                item->setData(qint64(INVALID_THREAD_ID), ConversationIdRole);
                item->setData(qint64(INVALID_DATE), DateRole);
                item->setData(rawPhoneNumber.toString(), SenderRole);
                appendRow(item);
            }
        }
    }
}

void ConversationListModel::createConversationForAddress(const QString& address)
{
    QStandardItem* item = new QStandardItem();
    const QString canonicalizedAddress = SmsHelper::canonicalizePhoneNumber(address);
    item->setText(canonicalizedAddress);

    QList<ConversationAddress> addresses;
    addresses.append(ConversationAddress(canonicalizedAddress));
    item->setData(QVariant::fromValue(addresses), AddressesRole);

    QString displayBody = i18n("%1", canonicalizedAddress);
    item->setData(displayBody, Qt::ToolTipRole);
    item->setData(false, MultitargetRole);
    item->setData(qint64(INVALID_THREAD_ID), ConversationIdRole);
    item->setData(qint64(INVALID_DATE), DateRole);
    item->setData(address, SenderRole);
    appendRow(item);
}

bool ConversationListModel::isPhoneNumberValid(const QString& number)
{
    return SmsHelper::isPhoneNumberValid(number);
}
