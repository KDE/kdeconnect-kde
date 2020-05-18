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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "conversationmessage.h"

#include <QLoggingCategory>
#include <QVariantMap>

Q_LOGGING_CATEGORY(CONVERSATION_MESSAGE_LOGGING_CATEGORY, "kdeconnect.interfaces.conversationmessage")

ConversationMessage::ConversationMessage(const QVariantMap& args)
    : m_eventField(args[QStringLiteral("event")].toInt()),
      m_body(args[QStringLiteral("body")].toString()),
      m_date(args[QStringLiteral("date")].toLongLong()),
      m_type(args[QStringLiteral("type")].toInt()),
      m_read(args[QStringLiteral("read")].toInt()),
      m_threadID(args[QStringLiteral("thread_id")].toLongLong()),
      m_uID(args[QStringLiteral("_id")].toInt())
{
    QString test = QLatin1String(args[QStringLiteral("addresses")].typeName());
    QVariantList jsonAddresses = args[QStringLiteral("addresses")].toList();
    for (const QVariant& addressField : jsonAddresses) {
        const auto& rawAddress = addressField.toMap();
        m_addresses.append(ConversationAddress(rawAddress[QStringLiteral("address")].value<QString>()));
    }
    QVariantMap::const_iterator subID_it = args.find(QStringLiteral("sub_id"));
    m_subID = subID_it == args.end() ? -1 : subID_it->toLongLong();
}

ConversationMessage::ConversationMessage (const qint32& eventField, const QString& body,
                                          const QList<ConversationAddress>& addresses, const qint64& date,
                                          const qint32& type, const qint32& read,
                                          const qint64& threadID,
                                          const qint32& uID,
                                          const qint64& subID)
    : m_eventField(eventField)
    , m_body(body)
    , m_addresses(addresses)
    , m_date(date)
    , m_type(type)
    , m_read(read)
    , m_threadID(threadID)
    , m_uID(uID)
    , m_subID(subID)
{
}

ConversationMessage::ConversationMessage(const ConversationMessage& other)
    : m_eventField(other.m_eventField)
    , m_body(other.m_body)
    , m_addresses(other.m_addresses)
    , m_date(other.m_date)
    , m_type(other.m_type)
    , m_read(other.m_read)
    , m_threadID(other.m_threadID)
    , m_uID(other.m_uID)
    , m_subID(other.m_subID)
{
}

ConversationMessage::~ConversationMessage() { }

ConversationMessage& ConversationMessage::operator=(const ConversationMessage& other)
{
    this->m_eventField = other.m_eventField;
    this->m_body = other.m_body;
    this->m_addresses = other.m_addresses;
    this->m_date = other.m_date;
    this->m_type = other.m_type;
    this->m_read = other.m_read;
    this->m_threadID = other.m_threadID;
    this->m_uID = other.m_uID;
    this->m_subID = other.m_subID;
    return *this;
}

ConversationMessage ConversationMessage::fromDBus(const QDBusVariant& var)
{
    QDBusArgument data = var.variant().value<QDBusArgument>();
    ConversationMessage message;
    data >> message;
    return message;
}

QVariantMap ConversationMessage::toVariant() const
{
    QVariantList addresses;
    for (const ConversationAddress& address : m_addresses) {
        addresses.push_back(address.toVariant());
    }

    return {
        {QStringLiteral("event"), m_eventField},
        {QStringLiteral("body"), m_body},
        {QStringLiteral("addresses"), addresses},
        {QStringLiteral("date"), m_date},
        {QStringLiteral("type"), m_type},
        {QStringLiteral("read"), m_read},
        {QStringLiteral("thread_id"), m_threadID},
        {QStringLiteral("_id"), m_uID},
        {QStringLiteral("sub_id"), m_subID}
    };
}

ConversationAddress::ConversationAddress(QString address)
    : m_address(address)
{}

ConversationAddress::ConversationAddress(const ConversationAddress& other)
    : m_address(other.address())
{}

ConversationAddress::~ConversationAddress()
{}

ConversationAddress& ConversationAddress::operator=(const ConversationAddress& other)
{
    this->m_address = other.m_address;
    return *this;
}

QList<ConversationAddress> ConversationAddress::listfromDBus(const QDBusVariant& var)
{
    QDBusArgument data = var.variant().value<QDBusArgument>();
    QList<ConversationAddress> addresses;
    data >> addresses;
    return addresses;
}

QVariantMap ConversationAddress::toVariant() const
{
    return {
        {QStringLiteral("address"), address()},
    };
}

void ConversationMessage::registerDbusType()
{
    qDBusRegisterMetaType<ConversationMessage>();
    qRegisterMetaType<ConversationMessage>();
    qDBusRegisterMetaType<ConversationAddress>();
    qRegisterMetaType<ConversationAddress>();
    qDBusRegisterMetaType<QList<ConversationAddress>>();
    qRegisterMetaType<QList<ConversationAddress>>();
}
