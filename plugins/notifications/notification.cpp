/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "notification.h"
#include "plugin_notification_debug.h"

#include <KNotification>
#include <QtGlobal>
#include <QIcon>
#include <QString>
#include <QUrl>
#include <QPixmap>
#include <KLocalizedString>
#include <QFile>
#include <knotifications_version.h>
#include <QJsonArray>

#include <core/filetransferjob.h>
#include <core/notificationserverinfo.h>

QMap<QString, FileTransferJob*> Notification::s_downloadsInProgress;

Notification::Notification(const NetworkPacket& np, const Device* device, QObject* parent)
    : QObject(parent)
    , m_imagesDir()
    , m_device(device)
{
    //Make a own directory for each user so no one can see each others icons
    QString username;
    #ifdef Q_OS_WIN
        username = QString::fromLatin1(qgetenv("USERNAME"));
    #else
        username = QString::fromLatin1(qgetenv("USER"));
    #endif

    m_imagesDir.setPath(QDir::temp().absoluteFilePath(QStringLiteral("kdeconnect_") + username));
    m_imagesDir.mkpath(m_imagesDir.absolutePath());
    QFile(m_imagesDir.absolutePath()).setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    m_ready = false;

    parseNetworkPacket(np);
    createKNotification(np);

    connect(m_notification, QOverload<unsigned int>::of(&KNotification::activated), this, [this] (unsigned int actionIndex) {
        // Do nothing for our own reply action
        if(!m_requestReplyId.isEmpty() && actionIndex == 1) {
            return;
        }

        // index 0 is the default action. Some notification servers send this even though they shouldn't.
        // https://bugs.kde.org/show_bug.cgi?id=427717
        if (actionIndex == 0) {
            return;
        }

        // Notification action indices start at 1
        Q_EMIT actionTriggered(m_internalId, m_actions[actionIndex - 1]);
    });
}

Notification::~Notification()
{
}

void Notification::dismiss()
{
    if (m_dismissable) {
        Q_EMIT dismissRequested(m_internalId);
    }
}

void Notification::show()
{
    m_ready = true;
    Q_EMIT ready();
    if (!m_silent) {
        m_notification->sendEvent();
    }
}

void Notification::update(const NetworkPacket& np)
{
    parseNetworkPacket(np);
    createKNotification(np);
}

void Notification::createKNotification(const NetworkPacket& np)
{
    if (!m_notification) {
        m_notification = new KNotification(QStringLiteral("notification"), KNotification::CloseOnTimeout, this);
        m_notification->setComponentName(QStringLiteral("kdeconnect"));
    }

    QString escapedTitle = m_title.toHtmlEscaped();
    QString escapedText = m_text.toHtmlEscaped();
    QString escapedTicker = m_ticker.toHtmlEscaped();

    if (NotificationServerInfo::instance().supportedHints().testFlag(NotificationServerInfo::X_KDE_DISPLAY_APPNAME)) {
        m_notification->setTitle(escapedTitle);
        m_notification->setText(escapedText);
        m_notification->setHint(QStringLiteral("x-kde-display-appname"), m_appName.toHtmlEscaped());
    } else {
        m_notification->setTitle(m_appName.toHtmlEscaped());

        if (m_title.isEmpty() && m_text.isEmpty()) {
            m_notification->setText(escapedTicker);
        } else if (m_appName == m_title) {
            m_notification->setText(escapedText);
        } else if (m_title.isEmpty()) {
            m_notification->setText(escapedText);
        } else if (m_text.isEmpty()) {
            m_notification->setText(escapedTitle);
        } else {
            m_notification->setText(escapedTitle + QStringLiteral(": ") + escapedText);
        }
    }

    m_notification->setHint(QStringLiteral("x-kde-origin-name"), m_device->name());

    if (!m_requestReplyId.isEmpty()) {
        m_actions.prepend(i18n("Reply"));
        connect(m_notification, &KNotification::action1Activated, this, &Notification::reply, Qt::UniqueConnection);
    }

    m_notification->setActions(m_actions);

    m_hasIcon = m_hasIcon && !m_payloadHash.isEmpty();

    if (!m_hasIcon) {
        show();
    } else {
        m_iconPath = m_imagesDir.absoluteFilePath(m_payloadHash);
        loadIcon(np);
    }
}

void Notification::loadIcon(const NetworkPacket& np)
{
    m_ready = false;

    if (QFileInfo::exists(m_iconPath)) {
        applyIcon();
        show();
    } else {
        FileTransferJob* fileTransferJob = s_downloadsInProgress.value(m_iconPath);
        if (!fileTransferJob) {
            fileTransferJob = np.createPayloadTransferJob(QUrl::fromLocalFile(m_iconPath));
            fileTransferJob->start();
            s_downloadsInProgress[m_iconPath] = fileTransferJob;
        }

        connect(fileTransferJob, &FileTransferJob::result, this, [this, fileTransferJob]{
            s_downloadsInProgress.remove(m_iconPath);
            if (fileTransferJob->error()) {
                qCDebug(KDECONNECT_PLUGIN_NOTIFICATION) << "Error in FileTransferJob: " << fileTransferJob->errorString();
            } else {
                applyIcon();
            }
            show();
        });
    }
}

void Notification::applyIcon()
{
    QPixmap icon(m_iconPath, "PNG");
    m_notification->setPixmap(icon);
}

void Notification::reply()
{
    Q_EMIT replyRequested();
}

void Notification::parseNetworkPacket(const NetworkPacket& np)
{
    m_internalId = np.get<QString>(QStringLiteral("id"));
    m_appName = np.get<QString>(QStringLiteral("appName"));
    m_ticker = np.get<QString>(QStringLiteral("ticker"));
    m_title = np.get<QString>(QStringLiteral("title"));
    m_text = np.get<QString>(QStringLiteral("text"));
    m_dismissable = np.get<bool>(QStringLiteral("isClearable"));
    m_hasIcon = np.hasPayload();
    m_silent = np.get<bool>(QStringLiteral("silent"));
    m_payloadHash = np.get<QString>(QStringLiteral("payloadHash"));
    m_requestReplyId = np.get<QString>(QStringLiteral("requestReplyId"), QString());

    m_actions.clear();

    const auto actions = np.get<QJsonArray>(QStringLiteral("actions"));
    for (const QJsonValue& value : actions) {
        m_actions.append(value.toString());
    }

}
