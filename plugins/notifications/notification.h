/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KNotification>
#include <QDir>
#include <QObject>
#include <QPointer>
#include <QString>

#include <core/device.h>
#include <core/networkpacket.h>

class Notification : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.notifications.notification")
    Q_PROPERTY(QString internalId READ internalId CONSTANT)
    Q_PROPERTY(QString appName READ appName NOTIFY ready)
    Q_PROPERTY(QString ticker READ ticker NOTIFY ready)
    Q_PROPERTY(QString title READ title NOTIFY ready)
    Q_PROPERTY(QString text READ text NOTIFY ready)
    Q_PROPERTY(QString iconPath READ iconPath NOTIFY ready)
    Q_PROPERTY(bool dismissable READ dismissable NOTIFY ready)
    Q_PROPERTY(bool hasIcon READ hasIcon NOTIFY ready)
    Q_PROPERTY(bool silent READ silent NOTIFY ready)
    Q_PROPERTY(QString replyId READ replyId NOTIFY ready)

public:
    Notification(const NetworkPacket &np, const Device *device, QObject *parent);

    QString internalId() const
    {
        return m_internalId;
    }
    QString appName() const
    {
        return m_appName;
    }
    QString ticker() const
    {
        return m_ticker;
    }
    QString title() const
    {
        return m_title;
    }
    QString text() const
    {
        return m_text;
    }
    QString iconPath() const
    {
        return m_iconPath;
    }
    bool dismissable() const
    {
        return m_dismissable;
    }
    QString replyId() const
    {
        return m_requestReplyId;
    }
    bool hasIcon() const
    {
        return m_hasIcon;
    }
    void show();
    bool silent() const
    {
        return m_silent;
    }
    void update(const NetworkPacket &np);
    bool isReady() const
    {
        return m_ready;
    }
    void createKNotification(const NetworkPacket &np);

public Q_SLOTS:
    Q_SCRIPTABLE void dismiss();
    Q_SCRIPTABLE void reply();
    Q_SCRIPTABLE void sendReply(const QString &message);

Q_SIGNALS:
    void dismissRequested(const QString &m_internalId);
    void replyRequested();
    Q_SCRIPTABLE void ready();
    void actionTriggered(const QString &key, const QString &action);
    void replied(const QString &message);

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
    QPointer<KNotification> m_notification;
    QDir m_imagesDir;
    bool m_silent;
    QString m_payloadHash;
    bool m_ready;
    QStringList m_actions;
    const Device *m_device;

    void parseNetworkPacket(const NetworkPacket &np);
    void loadIcon(const NetworkPacket &np);
    void applyIcon();

    static QMap<QString, FileTransferJob *> s_downloadsInProgress;
};
