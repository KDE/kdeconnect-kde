/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "notification.h"
#include "plugin_notifications_debug.h"

#include <KLocalizedString>
#include <KNotification>
#include <KNotificationReplyAction>
#include <QFile>
#include <QIcon>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QPixmap>
#include <QString>
#include <QUrl>
#include <QtGlobal>
#include <knotifications_version.h>

#include <core/filetransferjob.h>
#include <core/notificationserverinfo.h>

QMap<QString, FileTransferJob *> Notification::s_downloadsInProgress;

Notification::Notification(const NetworkPacket &np, const Device *device, QObject *parent)
    : QObject(parent)
    , m_imagesDir()
    , m_device(device)
{
    // Make a own directory for each user so no one can see each others icons
    QString username;
#ifdef Q_OS_WIN
    username = QString::fromLatin1(qgetenv("USERNAME"));
#else
    username = QString::fromLatin1(qgetenv("USER"));
#endif

    m_imagesDir.setPath(QDir::temp().absoluteFilePath(QStringLiteral("kdeconnect_") + username));
    m_imagesDir.mkpath(m_imagesDir.absolutePath());
    QFile(m_imagesDir.absolutePath()).setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);

    update(np);
}

void Notification::dismiss()
{
    if (m_contents.dismissable) {
        Q_EMIT dismissRequested(m_internalId);
    }
}

void Notification::show()
{
    // Emitting ready() makes every D-Bus client re-read the notification, so only do it if something changed
    if (m_lastPublishedContents != m_contents) {
        m_lastPublishedContents = m_contents;
        Q_EMIT ready();
    }
    // Silent notifications don't need a KNotification, unless there's already one shown that we need to update
    if (m_notification || !m_silent) {
        updateKNotification();
    }
    if (!m_silent) {
        m_notification->sendEvent();
    }
}

void Notification::updateKNotification()
{
    if (!m_notification) {
        m_notification = new KNotification(QStringLiteral("notification"), KNotification::CloseOnTimeout, this);
        m_notification->setComponentName(QStringLiteral("kdeconnect"));
        m_notification->setHint(QStringLiteral("resident"),
                                true); // This means the notification won't be deleted automatically, but only with KNotifications 5.81
    }

    const bool supportsMarkup = NotificationServerInfo::instance().supportedHints().testFlag(NotificationServerInfo::BODY_MARKUP);

    QString ticker = m_contents.ticker;
    QString title = m_contents.title;
    QString text = isConversation() ? getConversationMessages(supportsMarkup) : m_contents.text;

    if (supportsMarkup) {
        ticker = ticker.toHtmlEscaped();
        title = title.toHtmlEscaped();
        if (!isConversation()) {
            // getConversationMessages() already produces escaped text for conversations
            text = text.toHtmlEscaped();
        }
    }

    if (NotificationServerInfo::instance().supportedHints().testFlag(NotificationServerInfo::X_KDE_DISPLAY_APPNAME)) {
        m_notification->setTitle(m_contents.title);
        m_notification->setText(text);
        m_notification->setHint(QStringLiteral("x-kde-display-appname"), m_contents.appName);
    } else {
        m_notification->setTitle(m_contents.appName);

        if (title.isEmpty() && text.isEmpty()) {
            m_notification->setText(ticker);
        } else if (m_contents.appName == m_contents.title || title.isEmpty()) {
            m_notification->setText(text);
        } else if (text.isEmpty()) {
            m_notification->setText(title);
        } else {
            m_notification->setText(title + QStringLiteral(": ") + text);
        }
    }

    m_notification->setHint(QStringLiteral("x-kde-origin-name"), m_device->name());
    // Mark this notification so sendnotifications can filter it out and not echo it back to the device
    m_notification->setHint(QStringLiteral("x-kdeconnect-source-device"), m_device->name());

    if (!m_contents.requestReplyId.isEmpty()) {
        auto replyAction = std::make_unique<KNotificationReplyAction>(i18nc("@action:button", "Reply"));

        if (isGroupConversation()) {
            replyAction->setPlaceholderText(i18nc("@info:placeholder %1 is the name of a group chat", "Reply in %1…", m_contents.groupName));
        } else {
            replyAction->setPlaceholderText(i18nc("@info:placeholder", "Reply to %1…", isConversation() ? m_contents.title : m_contents.appName));
        }

        replyAction->setFallbackBehavior(KNotificationReplyAction::FallbackBehavior::UseRegularAction);
        QObject::connect(replyAction.get(), &KNotificationReplyAction::replied, this, &Notification::replied);
        QObject::connect(replyAction.get(), &KNotificationReplyAction::activated, this, &Notification::reply);
        m_notification->setReplyAction(std::move(replyAction));
    } else {
        m_notification->setReplyAction({});
    }

    m_notification->clearActions();
    for (const QString &actionId : std::as_const(m_contents.actions)) {
        KNotificationAction *action = m_notification->addAction(actionId);

        connect(action, &KNotificationAction::activated, this, [this, actionId] {
            Q_EMIT actionTriggered(m_internalId, actionId);
        });
    }

    m_notification->setPixmap(m_contents.iconPath.isEmpty() ? QPixmap() : QPixmap(m_contents.iconPath, "PNG"));
}

QString Notification::getConversationMessages(bool supportsMarkup) const
{
    QString conversation;
    // To avoid showing the sender of the first message twice (in title and text) at the start of the conversation.
    QString prevSender = m_contents.title;

    for (const QJsonValue &messageElement : m_contents.conversation) {
        const Message message = Message::fromObject(messageElement.toObject());
        if (message.sender != prevSender) {
            if (supportsMarkup) {
                conversation.append(QStringLiteral("<b>"));
                conversation.append(message.sender.toHtmlEscaped());
                conversation.append(QStringLiteral("</b>\n"));
            } else {
                conversation.append(message.sender);
                conversation.append(QStringLiteral(":\n"));
            }

            prevSender = message.sender;
        }
        conversation.append(supportsMarkup ? message.content.toHtmlEscaped() : message.content);
        conversation.append(supportsMarkup ? QStringLiteral("<br/>") : QStringLiteral("\n"));
    }
    return conversation.chopped(supportsMarkup ? 5 : 1);
}

void Notification::reply()
{
    Q_EMIT replyRequested();
}

void Notification::sendReply(const QString &message)
{
    Q_EMIT replied(message);
}

void Notification::update(const NetworkPacket &np)
{
    m_internalId = np.get<QString>(QStringLiteral("id"));
    m_silent = np.get<bool>(QStringLiteral("silent"));

    m_contents.appName = np.get<QString>(QStringLiteral("appName"));
    m_contents.ticker = np.get<QString>(QStringLiteral("ticker"));
    m_contents.title = np.get<QString>(QStringLiteral("title"));
    m_contents.text = np.get<QString>(QStringLiteral("text"));
    m_contents.groupName = np.get<QString>(QStringLiteral("groupName"), QString());
    m_contents.dismissable = np.get<bool>(QStringLiteral("isClearable"));
    m_payloadHash = np.get<QString>(QStringLiteral("payloadHash"));
    m_contents.requestReplyId = np.get<QString>(QStringLiteral("requestReplyId"), QString());

    m_contents.conversation = np.get<QJsonArray>(QStringLiteral("conversation"), QJsonArray());
    if (!m_contents.conversation.isEmpty()) {
        m_contents.title = isGroupConversation() ? m_contents.groupName : Message::fromObject(m_contents.conversation.at(0).toObject()).sender;
        bool supportsMarkup = true; // The QML app and plasmoid, which query this via D-Bus, support HTML.
        m_contents.text = getConversationMessages(supportsMarkup);
    }

    m_contents.actions.clear();
    const auto actions = np.get<QJsonArray>(QStringLiteral("actions"));
    for (const QJsonValue &value : actions) {
        m_contents.actions.append(value.toString());
    }

    m_contents.iconPath.clear();
    if (!m_payloadHash.isEmpty()) {
        const QString dir = QDir::cleanPath(m_imagesDir.absolutePath());
        const QString filePath = QDir::cleanPath(m_imagesDir.absoluteFilePath(m_payloadHash));
        if (filePath.startsWith(dir + QStringLiteral("/"))) {
            m_contents.iconPath = filePath;
        } else {
            qCWarning(KDECONNECT_PLUGIN_NOTIFICATIONS) << "Directory traversal detected";
        }
    }

    // The file already exists while it's being downloaded, so check for a download in progress before QFileInfo::exists
    FileTransferJob *fileTransferJob = s_downloadsInProgress.value(m_contents.iconPath);
    if (m_contents.iconPath.isEmpty() || (!fileTransferJob && QFileInfo::exists(m_contents.iconPath))) {
        show();
    } else {
        if (!fileTransferJob) {
            fileTransferJob = np.createPayloadTransferJob(QUrl::fromLocalFile(m_contents.iconPath));
            s_downloadsInProgress[m_contents.iconPath] = fileTransferJob;
            // Use the job as context, so the entry is removed even if no Notification is waiting for it anymore
            connect(fileTransferJob, &KJob::result, fileTransferJob, [path = m_contents.iconPath] {
                s_downloadsInProgress.remove(path);
            });
            fileTransferJob->start();
        }

        // Unique, since update() can call this again while the same icon is still downloading
        connect(fileTransferJob, &FileTransferJob::result, this, &Notification::iconDownloaded, Qt::UniqueConnection);
    }
}

void Notification::iconDownloaded(KJob *job)
{
    // Ignore downloads of an icon that a later update replaced
    if (static_cast<FileTransferJob *>(job)->destination() != QUrl::fromLocalFile(m_contents.iconPath)) {
        return;
    }
    if (job->error()) {
        qCDebug(KDECONNECT_PLUGIN_NOTIFICATIONS) << "Error in FileTransferJob: " << job->errorString();
        m_contents.iconPath.clear();
    }
    show();
}

#include "moc_notification.cpp"
