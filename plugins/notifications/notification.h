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

    QString internalId() const { return m_internalId; }
    QString appName() const { return m_appName; }
    QString ticker() const { return m_ticker; }
    QString title() const { return m_title; }
    QString text() const { return m_text; }
    QString iconPath() const { return m_iconPath; }
    bool dismissable() const { return m_dismissable; }
    QString replyId() const { return m_requestReplyId; }
    bool hasIcon() const { return m_hasIcon; }
    void show();
    bool silent() const { return m_silent; }
    void update(const NetworkPackage& np);
    bool isReady() const { return m_ready; }
    KNotification* createKNotification(bool update, const NetworkPackage& np);

public Q_SLOTS:
    Q_SCRIPTABLE void dismiss();
    Q_SCRIPTABLE void reply();
    void closed();

    Q_SIGNALS:
    void dismissRequested(const QString& m_internalId);
    void replyRequested();
    void ready();

private:
    QString m_internalId;
    QString m_appName;
    QString m_ticker;
    QString m_title;
    QString m_text;
    QString m_iconPath;
    QString m_requestReplyId;
    bool m_dismissable;
    bool m_hasIcon;
    KNotification* m_notification;
    QDir m_imagesDir;
    bool m_silent;
    bool m_closed;
    QString m_payloadHash;
    bool m_ready;

    void parseNetworkPackage(const NetworkPackage& np);
    void loadIcon(const NetworkPackage& np);
    void applyIcon();
    void applyNoIcon();
};

#endif
