/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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

#include "notification.h"

#include <KNotification>
#include <QIcon>
#include <QString>
#include <QUrl>
#include <QPixmap>
#include <KLocalizedString>

#include <core/filetransferjob.h>


Notification::Notification(const NetworkPackage& np, QObject* parent)
    : QObject(parent)
{
    m_imagesDir = QDir::temp().absoluteFilePath(QStringLiteral("kdeconnect"));
    m_imagesDir.mkpath(m_imagesDir.absolutePath());
    m_closed = false;

    parseNetworkPackage(np);
    createKNotification(false, np);
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
    if (!m_silent) {
        m_closed = false;
        m_notification->sendEvent();
    }
}

void Notification::applyIconAndShow()
{
    if (!m_silent) {
        QPixmap icon(m_iconPath, "PNG");
        m_notification->setPixmap(icon);
        show();
    }
}

void Notification::update(const NetworkPackage& np)
{
    parseNetworkPackage(np);
    createKNotification(!m_closed, np);
}

KNotification* Notification::createKNotification(bool update, const NetworkPackage& np)
{
    if (!update) {
        m_notification = new KNotification(QStringLiteral("notification"), KNotification::CloseOnTimeout, this);
        m_notification->setComponentName(QStringLiteral("kdeconnect"));
    }

    QString escapedTitle = m_title.toHtmlEscaped();
    QString escapedText = m_text.toHtmlEscaped();
    QString escapedTicker = m_ticker.toHtmlEscaped();

    m_notification->setTitle(m_appName.toHtmlEscaped());

    if (m_title.isEmpty() && m_text.isEmpty()) {
       m_notification->setText(escapedTicker);
    } else if (m_appName==m_title) {
        m_notification->setText(escapedText);
    } else if (m_title.isEmpty()){
         m_notification->setText(escapedText);
    } else if (m_text.isEmpty()){
         m_notification->setText(escapedTitle);
    } else {
        m_notification->setText(escapedTitle+": "+escapedText);
    }

    if (!m_hasIcon) {
        //HACK The only way to display no icon at all is trying to load a non-existant icon
        m_notification->setIconName(QString("not_a_real_icon"));
        show();
    } else {
        QString filename = m_payloadHash;

        if (filename.isEmpty()) {
            m_hasIcon = false;
        } else {
            m_iconPath = m_imagesDir.absoluteFilePath(filename);
            QUrl destinationUrl(m_iconPath);
            FileTransferJob* job = np.createPayloadTransferJob(destinationUrl);
            job->start();
            connect(job, &FileTransferJob::result, this, &Notification::applyIconAndShow);
        }
    }

    if(!m_requestReplyId.isEmpty()) {
        m_notification->setActions( QStringList(i18n("Reply")) );
        connect(m_notification, &KNotification::action1Activated, this, &Notification::reply);
    }

    connect(m_notification, &KNotification::closed, this, &Notification::closed);

    return m_notification;
}

void Notification::reply()
{
    Q_EMIT replyRequested();
}

void Notification::closed()
{
    m_closed = true;
}

void Notification::parseNetworkPackage(const NetworkPackage& np)
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
}
