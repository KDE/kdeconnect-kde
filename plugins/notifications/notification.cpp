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
    mImagesDir = QDir::temp().absoluteFilePath(QStringLiteral("kdeconnect"));
    mImagesDir.mkpath(mImagesDir.absolutePath());
    mClosed = false;

    parseNetworkPackage(np);
    createKNotification(false, np);
}

Notification::~Notification()
{

}

void Notification::dismiss()
{
    if (mDismissable) {
        Q_EMIT dismissRequested(mInternalId);
    }
}

void Notification::show()
{
    if (!mSilent) {
        mClosed = false;
        mNotification->sendEvent();
    }
}

void Notification::applyIconAndShow()
{
    if (!mSilent) {
        QPixmap icon(mIconPath, "PNG");
        mNotification->setPixmap(icon);
        show();
    }
}

void Notification::update(const NetworkPackage &np)
{
    parseNetworkPackage(np);
    createKNotification(!mClosed, np);
}

KNotification* Notification::createKNotification(bool update, const NetworkPackage &np)
{
    if (!update) {
        mNotification = new KNotification(QStringLiteral("notification"), KNotification::CloseOnTimeout, this);
        mNotification->setComponentName(QStringLiteral("kdeconnect"));
    }

    QString escapedTitle = mTitle.toHtmlEscaped();
    QString escapedText = mText.toHtmlEscaped();
    QString escapedTicker = mTicker.toHtmlEscaped();

    mNotification->setTitle(mAppName.toHtmlEscaped());

    if (mTitle.isEmpty() && mText.isEmpty()) {
       mNotification->setText(escapedTicker);
    } else if (mAppName==mTitle) {
        mNotification->setText(escapedText);
    } else if (mTitle.isEmpty()){
         mNotification->setText(escapedText);
    } else if (mText.isEmpty()){
         mNotification->setText(escapedTitle);
    } else {
        mNotification->setText(escapedTitle+": "+escapedText);
    }

    if (!mHasIcon) {
        mNotification->setIconName(QStringLiteral("preferences-desktop-notification"));
        show();
    } else {
        QString filename = mPayloadHash;

        if (filename.isEmpty()) {
            mHasIcon = false;
        } else {
            mIconPath = mImagesDir.absoluteFilePath(filename);
            QUrl destinationUrl(mIconPath);
            FileTransferJob* job = np.createPayloadTransferJob(destinationUrl);
            job->start();
            connect(job, &FileTransferJob::result, this, &Notification::applyIconAndShow);
        }
    }
    
    if(!mRequestReplyId.isEmpty()) {
        mNotification->setActions( QStringList(i18n("Reply")) );
        connect(mNotification, &KNotification::action1Activated, this, &Notification::reply);
    }            

    connect(mNotification, &KNotification::closed, this, &Notification::closed);

    return mNotification;
}

void Notification::reply()
{
    Q_EMIT replyRequested();
}

void Notification::closed()
{
    mClosed = true;
}

void Notification::parseNetworkPackage(const NetworkPackage &np)
{
    mInternalId = np.get<QString>(QStringLiteral("id"));
    mAppName = np.get<QString>(QStringLiteral("appName"));
    mTicker = np.get<QString>(QStringLiteral("ticker"));
    mTitle = np.get<QString>(QStringLiteral("title"));
    mText = np.get<QString>(QStringLiteral("text"));
    mDismissable = np.get<bool>(QStringLiteral("isClearable"));
    mHasIcon = np.hasPayload();
    mSilent = np.get<bool>(QStringLiteral("silent"));
    mPayloadHash = np.get<QString>(QStringLiteral("payloadHash"));
    mRequestReplyId = np.get<QString>(QStringLiteral("requestReplyId"), QString());
}

