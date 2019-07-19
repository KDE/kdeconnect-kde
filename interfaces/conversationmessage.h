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

#ifndef PLUGINS_TELEPHONY_CONVERSATIONMESSAGE_H_
#define PLUGINS_TELEPHONY_CONVERSATIONMESSAGE_H_

#include <QDBusMetaType>

#include "kdeconnectinterfaces_export.h"

class KDECONNECTINTERFACES_EXPORT ConversationMessage
{

public:
    // TYPE field values from Android
    enum Types {
        MessageTypeAll = 0,
        MessageTypeInbox = 1,
        MessageTypeSent = 2,
        MessageTypeDraft = 3,
        MessageTypeOutbox = 4,
        MessageTypeFailed = 5,
        MessageTypeQueued = 6,
    };

    /**
     * Values describing the possible type of events contained in a message
     * A message's eventField is constructed as a bitwise-OR of events
     * Any events which are unsupported should be ignored
     */
    enum Events {
        EventTextMessage = 0x1, // This message has a body field which contains pure, human-readable text
        EventMultiTarget = 0x2, // This is a multitarget (group) message which has an "addresses" field which is a list of participants in the group
    };

    /**
     * Build a new message from a keyword argument dictionary
     *
     * @param args mapping of field names to values as might be contained in a network packet containing a message
     */
    ConversationMessage(const QVariantMap& args = QVariantMap());

    ConversationMessage(const qint32& eventField, const QString& body, const QString& address,
                        const qint64& date, const qint32& type, const qint32& read,
                        const qint64& threadID, const qint32& uID);

    ConversationMessage(const ConversationMessage& other);
    ~ConversationMessage();
    ConversationMessage& operator=(const ConversationMessage& other);
    static void registerDbusType();

    qint32 eventField() const { return m_eventField; }
    QString body() const { return m_body; }
    QString address() const { return m_address; }
    qint64 date() const { return m_date; }
    qint32 type() const { return m_type; }
    qint32 read() const { return m_read; }
    qint64 threadID() const { return m_threadID; }
    qint32 uID() const { return m_uID; }

    QVariantMap toVariant() const;

    bool containsTextBody() const { return (eventField() & ConversationMessage::EventTextMessage); }
    bool isMultitarget() const { return (eventField() & ConversationMessage::EventMultiTarget); }

protected:
    /**
     * Bitwise OR of event flags
     * Unsupported flags shall cause the message to be ignored
     */
    qint32 m_eventField;

    /**
     * Body of the message
     */
    QString m_body;

    /**
     * Remote-side address of the message. Most likely a phone number, but may be an email address
     */
    QString m_address;

    /**
     * Date stamp (Unix epoch millis) associated with the message
     */
    qint64 m_date;

    /**
     * Type of the message. See the message.type enum
     */
    qint32 m_type;

    /**
     * Whether we have a read report for this message
     */
    qint32 m_read;

    /**
     * Tag which binds individual messages into a thread
     */
    qint64 m_threadID;

    /**
     * Value which uniquely identifies a message
     */
    qint32 m_uID;
};

Q_DECLARE_METATYPE(ConversationMessage);

#endif /* PLUGINS_TELEPHONY_CONVERSATIONMESSAGE_H_ */
