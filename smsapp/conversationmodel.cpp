/**
 * SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "conversationmodel.h"

#include <KLocalizedString>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "attachmentinfo.h"
#include "interfaces/conversationmessage.h"
#include "smshelper.h"

#include "sms_conversation_debug.h"

ConversationModel::ConversationModel(QObject *parent)
    : QStandardItemModel(parent)
    , m_conversationsInterface(nullptr)
{
    auto roles = roleNames();
    roles.insert(FromMeRole, "fromMe");
    roles.insert(DateRole, "date");
    roles.insert(SenderRole, "sender");
    roles.insert(AvatarRole, "avatar");
    roles.insert(AttachmentsRole, "attachments");
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
    if (m_threadId != INVALID_THREAD_ID && !m_deviceId.isEmpty()) {
        requestMoreMessages();
        m_thumbnailsProvider->clear();
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
        disconnect(m_conversationsInterface, &DeviceConversationsDbusInterface::conversationLoaded, this, &ConversationModel::handleConversationLoaded);
        disconnect(m_conversationsInterface, &DeviceConversationsDbusInterface::conversationCreated, this, &ConversationModel::handleConversationCreated);
        delete m_conversationsInterface;
    }

    m_deviceId = deviceId;

    m_conversationsInterface = new DeviceConversationsDbusInterface(deviceId, this);
    connect(m_conversationsInterface, &DeviceConversationsDbusInterface::conversationUpdated, this, &ConversationModel::handleConversationUpdate);
    connect(m_conversationsInterface, &DeviceConversationsDbusInterface::conversationLoaded, this, &ConversationModel::handleConversationLoaded);
    connect(m_conversationsInterface, &DeviceConversationsDbusInterface::conversationCreated, this, &ConversationModel::handleConversationCreated);

    connect(m_conversationsInterface, &DeviceConversationsDbusInterface::attachmentReceived, this, &ConversationModel::filePathReceived);

    QQmlApplicationEngine *engine = qobject_cast<QQmlApplicationEngine *>(QQmlEngine::contextForObject(this)->engine());
    m_thumbnailsProvider = dynamic_cast<ThumbnailsProvider *>(engine->imageProvider(QStringLiteral("thumbnailsProvider")));

    // Clear any previous data on device change
    m_thumbnailsProvider->clear();

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

void ConversationModel::requestMoreMessages(const quint32 &howMany)
{
    if (m_threadId == INVALID_THREAD_ID) {
        return;
    }
    const auto &numMessages = knownMessageIDs.size();
    m_conversationsInterface->requestConversation(m_threadId, numMessages, numMessages + howMany);
}

void ConversationModel::createRowFromMessage(const ConversationMessage &message, int pos)
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
    item->setData(message.isOutgoing(), FromMeRole);
    item->setData(message.date(), DateRole);
    item->setData(senderName, SenderRole);

    QList<QVariant> attachmentInfoList;
    const QList<Attachment> attachmentList = message.attachments();

    for (const Attachment &attachment : attachmentList) {
        AttachmentInfo attachmentInfo(attachment);
        attachmentInfoList.append(QVariant::fromValue(attachmentInfo));

        if (attachment.mimeType().startsWith(QLatin1String("image")) || attachment.mimeType().startsWith(QLatin1String("video"))) {
            // The message contains thumbnail as Base64 String, convert it back into image thumbnail
            const QByteArray byteArray = attachment.base64EncodedFile().toUtf8();
            QPixmap thumbnail;
            thumbnail.loadFromData(QByteArray::fromBase64(byteArray));

            m_thumbnailsProvider->addImage(attachment.uniqueIdentifier(), thumbnail.toImage());
        }
    }

    item->setData(attachmentInfoList, AttachmentsRole);

    insertRow(pos, item);
    knownMessageIDs.insert(message.uID());
}

void ConversationModel::handleConversationUpdate(const QDBusVariant &msg)
{
    ConversationMessage message = ConversationMessage::fromDBus(msg);

    if (message.threadID() != m_threadId) {
        // If a conversation which we are not currently viewing was updated, discard the information
        qCDebug(KDECONNECT_SMS_CONVERSATION_MODEL) << "Saw update for thread" << message.threadID() << "but we are currently viewing" << m_threadId;
        return;
    }
    createRowFromMessage(message, 0);
}

void ConversationModel::handleConversationCreated(const QDBusVariant &msg)
{
    ConversationMessage message = ConversationMessage::fromDBus(msg);

    if (m_threadId == INVALID_THREAD_ID && SmsHelper::isPhoneNumberMatch(m_addressList[0].address(), message.addresses().first().address())
        && !message.isMultitarget()) {
        m_threadId = message.threadID();
        createRowFromMessage(message, 0);
    }
}

void ConversationModel::handleConversationLoaded(qint64 threadID)
{
    if (threadID != m_threadId) {
        return;
    }
    // If we get this flag, it means that the phone will not be responding with any more messages
    // so we should not be showing a loading indicator
    Q_EMIT loadingFinished();
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
    requestMoreMessages();
}

#include "moc_conversationmodel.cpp"
