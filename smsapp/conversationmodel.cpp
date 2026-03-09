/**
 * SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "conversationmodel.h"

#include <KLocalizedString>

#include <QApplication>
#include <QMessageBox>

#include "attachmentinfo.h"
#include "models/conversationmessage.h"
#include "smshelper.h"

#include "sms_conversation_debug.h"

ConversationModel::ConversationModel(QObject *parent)
    : QStandardItemModel(parent)
    , m_conversationsInterface(nullptr)
{
    auto roles = roleNames();
    roles.insert(Roles::FromMeRole, "fromMe");
    roles.insert(Roles::DateRole, "date");
    roles.insert(Roles::SenderRole, "sender");
    roles.insert(Roles::AvatarRole, "avatar");
    roles.insert(Roles::AttachmentsRole, "attachments");
    setItemRoleNames(roles);
}

ConversationModel::~ConversationModel()
{
}

qint64 ConversationModel::threadId() const
{
    return m_threadId;
}

void ConversationModel::setThreadId(const qint64 &threadId)
{
    if (m_threadId == threadId)
        return;

    m_threadId = threadId;
    clear();
    knownMessageIDs.clear();
    this->lastRequestedMessageIndex = std::nullopt;
    if (m_threadId != INVALID_THREAD_ID && !m_deviceId.isEmpty()) {
        requestMessages(RequestMessageRole::FetchMore);
        if (m_thumbnailsProvider) {
            (*m_thumbnailsProvider)->clear();
        }
    }
}

void ConversationModel::setDeviceId(const QString &deviceId)
{
    if (deviceId == m_deviceId)
        return;

    qCDebug(KDECONNECT_SMS_CONVERSATION_MODEL) << "setDeviceId"
                                               << "of" << this;
    if (m_conversationsInterface) {
        disconnect(m_conversationsInterface, &DeviceConversationsDbusInterface::conversationUpdated, this, &ConversationModel::handleConversationUpdate);
        disconnect(m_conversationsInterface, &DeviceConversationsDbusInterface::conversationCreated, this, &ConversationModel::handleConversationCreated);
        delete m_conversationsInterface;
    }

    m_deviceId = deviceId;

    m_conversationsInterface = new DeviceConversationsDbusInterface(deviceId, this);
    connect(m_conversationsInterface, &DeviceConversationsDbusInterface::conversationUpdated, this, &ConversationModel::handleConversationUpdate);
    connect(m_conversationsInterface, &DeviceConversationsDbusInterface::conversationCreated, this, &ConversationModel::handleConversationCreated);

    connect(m_conversationsInterface, &DeviceConversationsDbusInterface::attachmentReceived, this, &ConversationModel::filePathReceived);

    m_thumbnailsProvider = ThumbnailsProvider::getInContextForObject(this);
    if (!m_thumbnailsProvider) {
        // This is very unlikely to fail, so if it does, we ought to log a scary error!
        qCritical("Failed to load thumbnails provider, something didn't get initialised properly!");
        qCritical("Thumbnails will not be shown.");
    }

    // Clear any previous thumbnails on device change, if we have a thumbnailsProvider
    if (m_thumbnailsProvider) {
        (*m_thumbnailsProvider)->clear();
    }

    Q_EMIT deviceIdChanged(deviceId);
}

void ConversationModel::setAddressList(const QList<ConversationAddress> &addressList)
{
    m_addressList = addressList;
}

bool ConversationModel::sendReplyToConversation(const QString &textMessage, QList<QUrl> attachmentUrls)
{
    QVariantList fileUrls;
    for (const auto &url : attachmentUrls) {
        fileUrls << QVariant::fromValue(url.toLocalFile());
    }

    m_conversationsInterface->replyToConversation(m_threadId, textMessage, fileUrls);
    return true;
}

bool ConversationModel::startNewConversation(const QString &textMessage, const QList<ConversationAddress> &addressList, QList<QUrl> attachmentUrls)
{
    QVariantList addresses;

    for (const auto &address : addressList) {
        addresses << QVariant::fromValue(address);
    }

    QVariantList fileUrls;
    for (const auto &url : attachmentUrls) {
        fileUrls << QVariant::fromValue(url.toLocalFile());
    }

    m_conversationsInterface->sendWithoutConversation(addresses, textMessage, fileUrls);
    return true;
}

void ConversationModel::requestMessages(enum RequestMessageRole role)
{
    constexpr qint64 requestBatchSize = 25;
    constexpr qint64 maxRequestBatchSize = requestBatchSize * 10;

    if (m_threadId == INVALID_THREAD_ID) {
        return;
    }

    qCDebug(KDECONNECT_SMS_CONVERSATION_MODEL) << "RequestMessageRole: " << role;

    const auto &numMessages = knownMessageIDs.size();
    bool fetchedEverything = !lastRequestedMessageIndex || numMessages > lastRequestedMessageIndex.value();

    if (fetchedEverything) {
        if (role == RequestMessageRole::UiTimer) {
            qCDebug(KDECONNECT_SMS_CONVERSATION_MODEL) << "Nothing to be done for the " << role << " role";
            return;
        }
        this->lastRequestedMessageIndex = lastRequestedMessageIndex.value_or(-1) + requestBatchSize;
    } else {
        if (role == RequestMessageRole::UiPrefetch) {
            qCDebug(KDECONNECT_SMS_CONVERSATION_MODEL) << "Waiting for messages, ignoring " << role << " prefetch";
            return;
        } else if (role == RequestMessageRole::FetchMore) {
            qCDebug(KDECONNECT_SMS_CONVERSATION_MODEL) << "Waiting for messages but adding +1 anyway";
            this->lastRequestedMessageIndex = lastRequestedMessageIndex.value_or(-1) + 1;
        }
    }

    if (this->lastRequestedMessageIndex.value() - numMessages > maxRequestBatchSize) {
        // Maybe the UI is fetching too many entries at once, or maybe the model is not responding,
        // because there are no more messages, or for some other reason.
        // This check is mainly to avoid any potential overflows
        this->lastRequestedMessageIndex = maxRequestBatchSize;
        qCDebug(KDECONNECT_SMS_CONVERSATION_MODEL) << "Reached a maximum number of pending messages";
    }

    qCDebug(KDECONNECT_SMS_CONVERSATION_MODEL) << "Fetching from:" << numMessages << " to: " << this->lastRequestedMessageIndex.value() + 1;
    m_conversationsInterface->requestConversation(m_threadId, numMessages, this->lastRequestedMessageIndex.value() + 1);
}

void ConversationModel::createRowFromMessage(const ConversationMessage &message)
{
    if (message.threadID() != m_threadId) {
        // Because of the asynchronous nature of the current implementation of this model, if the
        // user clicks quickly between threads or for some other reason a message comes when we're
        // not expecting it, we should not display it in the wrong place
        qCDebug(KDECONNECT_SMS_CONVERSATION_MODEL) << "Got a message for a thread" << message.threadID() << "but we are currently viewing" << m_threadId
                                                   << "Discarding.";
        return;
    }

    if (knownMessageIDs.contains(message.uID())) {
        qCDebug(KDECONNECT_SMS_CONVERSATION_MODEL) << "Ignoring duplicate message with ID" << message.uID();
        return;
    }

    ConversationAddress sender;
    if (!message.addresses().isEmpty()) {
        sender = message.addresses().first();
    } else {
        qCDebug(KDECONNECT_SMS_CONVERSATION_MODEL) << "Conversation with ID " << message.threadID() << " did not have any addresses";
    }

    QString senderName = message.isIncoming() ? SmsHelper::getTitleForAddresses({sender}) : QString();
    QString displayBody = message.body();

    auto item = new QStandardItem;
    item->setText(displayBody);
    item->setData(message.isOutgoing(), Roles::FromMeRole);
    item->setData(message.date(), Roles::DateRole);
    item->setData(senderName, Roles::SenderRole);

    QList<QVariant> attachmentInfoList;
    const QList<Attachment> attachmentList = message.attachments();

    for (const Attachment &attachment : attachmentList) {
        AttachmentInfo attachmentInfo(attachment);
        attachmentInfoList.append(QVariant::fromValue(attachmentInfo));

        // Only compute thumbnails if we have a working thumbnailsProvider
        if (!m_thumbnailsProvider) {
            continue;
        }

        if (attachment.mimeType().startsWith(QLatin1String("image")) || attachment.mimeType().startsWith(QLatin1String("video"))) {
            // The message contains thumbnail as Base64 String, convert it back into image thumbnail
            const QByteArray byteArray = attachment.base64EncodedFile().toUtf8();
            QPixmap thumbnail;
            thumbnail.loadFromData(QByteArray::fromBase64(byteArray));

            // Unconditionally dereferencing here is fine, because we check that we have a
            // thumbnailsProvider above this `if` case.
            (*m_thumbnailsProvider)->addIcon(attachment.uniqueIdentifier(), QIcon(thumbnail));
        }
    }

    item->setData(attachmentInfoList, Roles::AttachmentsRole);

    knownMessageIDs.insert(message.uID());

    // With the current design of the system, this should never happen, but if somehow it does,
    // the event won't cause any issues
    if (!lastRequestedMessageIndex || knownMessageIDs.size() > lastRequestedMessageIndex.value() + 1) {
        qCDebug(KDECONNECT_SMS_CONVERSATION_MODEL) << "More messages arrived then requested";
        lastRequestedMessageIndex = knownMessageIDs.size() - 1;
    }

    // It is important to sort the items manually, so we know when a new message has arrived,
    // and the UI can react accordingly
    if (rowCount() > 0 && this->item(0)->data(Roles::DateRole).toLongLong() < message.date()) {
        Q_EMIT gotNewMessage();
        insertRow(0, item);
    } else {
        appendRow(item);
    }
}

void ConversationModel::handleConversationUpdate(const QDBusVariant &msg)
{
    ConversationMessage message = ConversationMessage::fromDBus(msg);

    if (message.threadID() != m_threadId) {
        // If a conversation which we are not currently viewing was updated, discard the information
        qCDebug(KDECONNECT_SMS_CONVERSATION_MODEL) << "Saw update for thread" << message.threadID() << "but we are currently viewing" << m_threadId;
        return;
    }
    createRowFromMessage(message);
}

void ConversationModel::handleConversationCreated(const QDBusVariant &msg)
{
    ConversationMessage message = ConversationMessage::fromDBus(msg);

    if (m_threadId == INVALID_THREAD_ID && SmsHelper::isPhoneNumberMatch(m_addressList[0].address(), message.addresses().first().address())
        && !message.isMultitarget()) {
        m_threadId = message.threadID();
        createRowFromMessage(message);
    }
}

QString ConversationModel::getCharCountInfo(const QString &message) const
{
    SmsCharCount count = SmsHelper::getCharCount(message);

    if (count.messages > 1) {
        // Show remaining char count and message count
        return QString::number(count.remaining) + QLatin1Char('/') + QString::number(count.messages);
    }
    if (count.messages == 1 && count.remaining < 10) {
        // Show only remaining char count
        return QString::number(count.remaining);
    } else {
        // Do not show anything
        return QString();
    }
}

void ConversationModel::requestAttachmentPath(const qint64 &partID, const QString &uniqueIdentifier)
{
    m_conversationsInterface->requestAttachmentFile(partID, uniqueIdentifier);
}

bool ConversationModel::canFetchMore(const QModelIndex & /* parent */) const
{
    // TODO: Is there any way to know when we're at the end of the conversation and can't fetch more?
    //       Would be neat to show an indication to the user in such a case (and stop fetching more data)
    //       but would also likely require sending that information from the phone, as I don't see an
    //       obvious way to know that using currently available information...
    return true;
}

void ConversationModel::fetchMore(const QModelIndex & /* parent */)
{
    requestMessages(RequestMessageRole::FetchMore);
}

#include "moc_conversationmodel.cpp"
