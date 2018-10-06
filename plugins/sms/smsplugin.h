/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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

#ifndef SMSPLUGIN_H
#define SMSPLUGIN_H

#include <QObject>

#include <core/kdeconnectplugin.h>

#include "conversationsdbusinterface.h"
#include "interfaces/conversationmessage.h"

#include "sendreplydialog.h"

/**
 * Packet used to indicate a batch of messages has been pushed from the remote device
 *
 * The body should contain the key "messages" mapping to an array of messages
 *
 * For example:
 * { "messages" : [
 *   {  "event" : "sms",
 *      "messageBody" : "Hello",
 *      "phoneNumber" : "2021234567",
 *      "messageDate" : "1518846484880",
 *      "messageType" : "2",
 *      "threadID" : "132"
 *    },
 *    { ... },
 *     ...
 *   ]
 * }
 */
#define PACKET_TYPE_SMS_MESSAGES QStringLiteral("kdeconnect.sms.messages")

/**
 * Packet sent to request a message be sent
 *
 * This will almost certainly need to be replaced or augmented to support MMS,
 * but be sure the Android side remains compatible with old desktop apps!
 *
 * The body should look like so:
 * { "sendSms": true,
 *   "phoneNumber": "542904563213",
 *   "messageBody": "Hi mom!"
 * }
 */
#define PACKET_TYPE_SMS_REQUEST QStringLiteral("kdeconnect.sms.request")

/**
 * Packet sent to request the most-recent message in each conversations on the device
 *
 * The request packet shall contain no body
 */
#define PACKET_TYPE_SMS_REQUEST_CONVERSATIONS QStringLiteral("kdeconnect.sms.request_conversations")

/**
 * Packet sent to request all the messages in a particular conversation
 *
 * The body should contain the key "threadID" mapping to the threadID being requested
 * For example:
 * { "threadID": 203 }
 */
#define PACKET_TYPE_SMS_REQUEST_CONVERSATION QStringLiteral("kdeconnect.sms.request_conversation")

Q_DECLARE_LOGGING_CATEGORY(KDECONNECT_PLUGIN_SMS)

class Q_DECL_EXPORT SmsPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.sms")

public:
    explicit SmsPlugin(QObject* parent, const QVariantList& args);
    ~SmsPlugin() override;

    bool receivePacket(const NetworkPacket& np) override;
    void connected() override {}

    QString dbusPath() const override;

public Q_SLOTS:
    Q_SCRIPTABLE void sendSms(const QString& phoneNumber, const QString& messageBody);

    /**
     * Send a request to the remote for all of its conversations
     */
    Q_SCRIPTABLE void requestAllConversations();

    /**
     * Send a request to the remote for a particular conversation
     */
    Q_SCRIPTABLE void requestConversation(const QString& conversationID) const;

private:

    /**
     * Send to the telepathy plugin if it is available
     */
    void forwardToTelepathy(const ConversationMessage& message);

    /**
     * Handle a packet which contains many messages, such as PACKET_TYPE_TELEPHONY_MESSAGE
     */
    bool handleBatchMessages(const NetworkPacket& np);

    QDBusInterface m_telepathyInterface;
    ConversationsDbusInterface* m_conversationInterface;

};

#endif
