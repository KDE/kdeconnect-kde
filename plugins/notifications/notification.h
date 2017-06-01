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

#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <QObject>
#include <QString>
#include <KNotification>
#include <QDir>

#include <core/networkpackage.h>

class Notification
    : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.notifications.notification")
    Q_PROPERTY(QString internalId READ internalId)
    Q_PROPERTY(QString appName READ appName)
    Q_PROPERTY(QString ticker READ ticker)
    Q_PROPERTY(QString title READ title)
    Q_PROPERTY(QString text READ text)
    Q_PROPERTY(QString iconPath READ iconPath)
    Q_PROPERTY(bool dismissable READ dismissable)
    Q_PROPERTY(bool hasIcon READ hasIcon)
    Q_PROPERTY(bool silent READ silent)
    Q_PROPERTY(QString replyId READ replyId)

public:
    Notification(const NetworkPackage& np, QObject* parent);
    ~Notification() override;

    QString internalId() const { return mInternalId; }
    QString appName() const { return mAppName; }
    QString ticker() const { return mTicker; }
    QString title() const { return mTitle; }
    QString text() const { return mText; }
    QString iconPath() const { return mIconPath; }
    bool dismissable() const { return mDismissable; }
    QString replyId() const { return mRequestReplyId; }
    bool hasIcon() const { return mHasIcon; }
    void show();
    bool silent() const { return mSilent; }
    void update(const NetworkPackage &np);
    KNotification* createKNotification(bool update, const NetworkPackage &np);

public Q_SLOTS:
    Q_SCRIPTABLE void dismiss();
    Q_SCRIPTABLE void applyIconAndShow();
    Q_SCRIPTABLE void reply();
    void closed();

    Q_SIGNALS:
    void dismissRequested(const QString& mInternalId);
    void replyRequested();

private:
    QString mInternalId;
    QString mAppName;
    QString mTicker;
    QString mTitle;
    QString mText;
    QString mIconPath;
    QString mRequestReplyId;
    bool mDismissable;
    bool mHasIcon;
    KNotification* mNotification;
    QDir mImagesDir;
    bool mSilent;
    bool mClosed;
    QString mPayloadHash;

    void parseNetworkPackage(const NetworkPackage& np);
};

#endif
