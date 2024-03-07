/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>

#include <core/kdeconnectplugin.h>

#include "conversationsdbusinterface.h"
#include "interfaces/conversationmessage.h"

/**
 * Packet used to indicate a batch of messages has been pushed from the remote device
 *
 * The body should contain the key "messages" mapping to an array of messages
 *
 * For example:
 * {
 *   "version": 2                     // This is the second version of this packet type and
 *                                    // version 1 packets (which did not carry this flag)
 *                                    // are incompatible with the new format
 *   "messages" : [
 *   { "event"     : 1,               // 32-bit field containing a bitwise-or of event flags
 *                                    // See constants declared in SMSHelper.Message for defined
 *                                    // values and explanations
 *     "body"      : "Hello",         // Text message body
 *     "addresses": <List<Address>>   // List of Address objects, one for each participant of the conversation
 *                                    // The user's Address is excluded so:
 *                                    // If this is a single-target message, there will only be one
 *                                    // Address (the other party)
 *                                    // If this is an incoming multi-target message, the first Address is the
 *                                    // sender and all other addresses are other parties to the conversation
 *                                    // If this is an outgoing multi-target message, the sender is implicit
 *                                    // (the user's phone number) and all Addresses are recipients
 *     "date"      : "1518846484880", // Timestamp of the message
 *     "type"      : "2",   // Compare with Android's
 *                          // Telephony.TextBasedSmsColumns.MESSAGE_TYPE_*
 *     "thread_id" : 132    // Thread to which the message belongs
 *     "read"      : true   // Boolean representing whether a message is read or unread
 *   },
 *   { ... },
 *   ...
 * ]
 *
 * The following optional fields of a message object may be defined
 * "sub_id": <int> // Android's subscriber ID, which is basically used to determine which SIM card the message
 *                 // belongs to. This is mostly useful when attempting to reply to an SMS with the correct
 *                 // SIM card using PACKET_TYPE_SMS_REQUEST.
 *                 // If this value is not defined or if it does not match a valid subscriber_id known by
 *                 // Android, we will use whatever subscriber ID Android gives us as the default
 *
 * "attachments": <List<Attachment>>    // List of Attachment objects, one for each attached file in the message.
 *
 * An Attachment object looks like:
 * {
 *     "part_id": <long>                // part_id of the attachment used to read the file from MMS database
 *     "mime_type": <int>               // contains the mime type of the file (image, video, audio, etc.)
 *     "encoded_thumbnail": <String>    // Optional base64-encoded thumbnail preview of the content for types which support it
 *     "unique_identifier": <String>    // Unique name of te file
 * }
 *
 * An Address object looks like:
 * {
 *     "address": <String> // Address (phone number, email address, etc.) of this object
 * }
 */
#define PACKET_TYPE_SMS_MESSAGES QStringLiteral("kdeconnect.sms.messages")

/**
 * Packet sent to request a message be sent
 *
 * The body should look like so:
 * {
 *   "version": 2,                     // The version of the packet being sent. Compare to SMS_REQUEST_PACKET_VERSION before attempting to handle.
 *   "addresses": <List<Addresses>>,   // The one or many targets of this message
 *   "messageBody": "Hi mom!",         // Plain-text string to be sent as the body of the message
 *   "attachments": <List<Attachment>>,// Send one or more attachments with this message. See AttachmentContainer documentation for formatting. (Optional)
 *   "sub_id": 3859358340534           // Some magic number which tells Android which SIM card to use (Optional, if omitted, sends with the default SIM card)
 * }
 *
 * An AttachmentContainer object looks like:
 * {
 *   "fileName": <String>             // Name of the file
 *   "base64EncodedFile": <String>    // Base64 encoded file
 *   "mimeType": <String>             // File type (eg: image/jpg, video/mp4 etc.)
 * }
 *
 */
#define PACKET_TYPE_SMS_REQUEST QStringLiteral("kdeconnect.sms.request")
#define SMS_REQUEST_PACKET_VERSION 2 // We *send* packets of this version

/**
 * Packet sent to request the most-recent message in each conversations on the device
 *
 * The request packet shall contain no body
 */
#define PACKET_TYPE_SMS_REQUEST_CONVERSATIONS QStringLiteral("kdeconnect.sms.request_conversations")

/**
 * Packet sent to request all the messages in a particular conversation
 *
 * The following fields are available:
 * "threadID": <long>            // (Required) ThreadID to request
 * "rangeStartTimestamp": <long> // (Optional) Millisecond epoch timestamp indicating the start of the range from which to return messages
 * "numberToRequest": <long>     // (Optional) Number of messages to return, starting from rangeStartTimestamp.
 *                               // May return fewer than expected if there are not enough or more than expected if many
 *                               // messages have the same timestamp.
 */
#define PACKET_TYPE_SMS_REQUEST_CONVERSATION QStringLiteral("kdeconnect.sms.request_conversation")

/**
 * Packet sent to request an attachment file in a particular message of a conversation
 *
 * The body should look like so:
 * "part_id": <long>                 // Part id of the attachment
 * "unique_identifier": <String>     // It can be any hash code or unique name of the file
 */
#define PACKET_TYPE_SMS_REQUEST_ATTACHMENT QStringLiteral("kdeconnect.sms.request_attachment")

/**
 * Packet used to send original attachment file from mms database to desktop
 * <p>
 * The following fields are available:
 * "thread_id": <long>      // Thread to which the attachment belongs
 * "filename": <String>     // Name of the attachment file in the database
 */
#define PACKET_TYPE_SMS_ATTACHMENT_FILE QStringLiteral("kdeconnect.sms.attachment_file")

Q_DECLARE_LOGGING_CATEGORY(KDECONNECT_PLUGIN_SMS)

class SmsPlugin : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.sms")

public:
    explicit SmsPlugin(QObject *parent, const QVariantList &args);
    ~SmsPlugin() override;

    void receivePacket(const NetworkPacket &np) override;

    QString dbusPath() const override;

public Q_SLOTS:
    Q_SCRIPTABLE void sendSms(const QVariantList &addresses, const QString &textMessage, const QVariantList &attachmentUrls, const qint64 subID = -1);

    /**
     * Send a request to the remote for all of its conversations
     */
    Q_SCRIPTABLE void requestAllConversations();

    /**
     * Send a request to the remote for a particular conversation
     *
     * @param conversationID The conversation to query
     * @param rangeStartTimestamp Return messages with timestamp >= this value. Value <= 0 indicates no limit.
     * @param numberToRequest Request this many messages. May return more, may return less. Value <= 0 indicates no limit.
     */
    Q_SCRIPTABLE void requestConversation(const qint64 conversationID, const qint64 rangeStartTimestamp = -1, const qint64 numberToRequest = -1) const;

    Q_SCRIPTABLE void launchApp();

    /**
     * Send a request to the remote device for a particulr attachment file
     */
    Q_SCRIPTABLE void requestAttachment(const qint64 &partID, const QString &uniqueIdentifier);

    /**
     * Searches the requested file in the application's cache directory,
     * if not found then sends the request to remote device
     */
    Q_SCRIPTABLE void getAttachment(const qint64 &partID, const QString &uniqueIdentifier);

private:
    /**
     * Send to the telepathy plugin if it is available
     */
    void forwardToTelepathy(const ConversationMessage &message);

    /**
     * Handle a packet which contains many messages, such as PACKET_TYPE_TELEPHONY_MESSAGE
     */
    bool handleBatchMessages(const NetworkPacket &np);

    /**
     * Handle a packet of type PACKET_TYPE_SMS_ATTACHMENT_FILE which contains an attachment file
     */
    bool handleSmsAttachmentFile(const NetworkPacket &np);

    /**
     * Encode a local file so it can be sent to the remote device as part of an MMS message.
     */
    Attachment createAttachmentFromUrl(const QString &url);

    QDBusInterface m_telepathyInterface;
    ConversationsDbusInterface *m_conversationInterface;
};
