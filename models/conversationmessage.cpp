/**
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "conversationmessage.h"

#include <QVariantMap>

ConversationMessage::ConversationMessage(const QVariantMap &args)
    : m_eventField(args[QStringLiteral("event")].toInt())
    , m_body(args[QStringLiteral("body")].toString())
    , m_date(args[QStringLiteral("date")].toLongLong())
    , m_type(args[QStringLiteral("type")].toInt())
    , m_read(args[QStringLiteral("read")].toInt())
    , m_threadID(args[QStringLiteral("thread_id")].toLongLong())
    , m_uID(args[QStringLiteral("_id")].toInt())
{
    QVariantList jsonAddresses = args[QStringLiteral("addresses")].toList();
    for (const QVariant &addressField : jsonAddresses) {
        const auto &rawAddress = addressField.toMap();
        m_addresses.append(ConversationAddress(rawAddress[QStringLiteral("address")].value<QString>()));
    }
    QVariantMap::const_iterator subID_it = args.find(QStringLiteral("sub_id"));
    m_subID = subID_it == args.end() ? -1 : subID_it->toLongLong();

    if (args.contains(QStringLiteral("attachments"))) {
        QVariant attachment = args.value(QStringLiteral("attachments"));
        const QVariantList jsonAttachments = attachment.toList();
        for (const QVariant &attachmentField : jsonAttachments) {
            const auto &rawAttachment = attachmentField.toMap();
            m_attachments.append(Attachment(rawAttachment[QStringLiteral("part_id")].value<qint64>(),
                                            rawAttachment[QStringLiteral("mime_type")].value<QString>(),
                                            rawAttachment[QStringLiteral("encoded_thumbnail")].value<QString>(),
                                            rawAttachment[QStringLiteral("unique_identifier")].value<QString>()));
        }
    }
}

ConversationMessage::ConversationMessage(const qint32 &eventField,
                                         const QString &body,
                                         const QList<ConversationAddress> &addresses,
                                         const qint64 &date,
                                         const qint32 &type,
                                         const qint32 &read,
                                         const qint64 &threadID,
                                         const qint32 &uID,
                                         const qint64 &subID,
                                         const QList<Attachment> &attachments)
    : m_eventField(eventField)
    , m_body(body)
    , m_addresses(addresses)
    , m_date(date)
    , m_type(type)
    , m_read(read)
    , m_threadID(threadID)
    , m_uID(uID)
    , m_subID(subID)
    , m_attachments(attachments)
{
}

ConversationMessage ConversationMessage::fromDBus(const QDBusVariant &var)
{
    QDBusArgument data = var.variant().value<QDBusArgument>();
    ConversationMessage message;
    data >> message;
    return message;
}

ConversationAddress::ConversationAddress(QString address)
    : m_address(address)
{
}

bool ConversationMessage::isOutgoing() const
{
    return type() == MessageTypeSent || type() == MessageTypeOutbox || type() == MessageTypeDraft || type() == MessageTypeFailed || type() == MessageTypeQueued;
}

Attachment::Attachment(qint64 partID, QString mimeType, QString base64EncodedFile, QString uniqueIdentifier)
    : m_partID(partID)
    , m_mimeType(mimeType)
    , m_base64EncodedFile(base64EncodedFile)
    , m_uniqueIdentifier(uniqueIdentifier)
{
}

void ConversationMessage::registerDbusType()
{
    qDBusRegisterMetaType<ConversationMessage>();
    qRegisterMetaType<ConversationMessage>();
    qDBusRegisterMetaType<ConversationAddress>();
    qRegisterMetaType<ConversationAddress>();
    qDBusRegisterMetaType<QList<ConversationAddress>>();
    qRegisterMetaType<QList<ConversationAddress>>();
    qDBusRegisterMetaType<Attachment>();
    qRegisterMetaType<Attachment>();
}
