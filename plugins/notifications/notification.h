/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KNotification>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>
#include <optional>

#include <core/device.h>
#include <core/networkpacket.h>

class KJob;

class Notification : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.notifications.notification")
    Q_PROPERTY(QString internalId READ internalId CONSTANT)
    Q_PROPERTY(QString appName READ appName NOTIFY ready)
    Q_PROPERTY(QString ticker READ ticker NOTIFY ready)
    Q_PROPERTY(QString title READ title NOTIFY ready)
    Q_PROPERTY(QString text READ text NOTIFY ready)
    Q_PROPERTY(QString groupName READ groupName NOTIFY ready)
    Q_PROPERTY(bool isConversation READ isConversation NOTIFY ready)
    Q_PROPERTY(bool isGroupConversation READ isGroupConversation NOTIFY ready)
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
        return m_contents.appName;
    }
    QString ticker() const
    {
        return m_contents.ticker;
    }
    QString title() const
    {
        return m_contents.title;
    }
    QString text() const
    {
        return m_contents.text;
    }
    QString groupName() const
    {
        return m_contents.groupName;
    }
    QString iconPath() const
    {
        return m_contents.iconPath;
    }
    bool dismissable() const
    {
        return m_contents.dismissable;
    }
    QString replyId() const
    {
        return m_contents.requestReplyId;
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
    bool isConversation() const
    {
        return !m_contents.conversation.isEmpty();
    }
    bool isGroupConversation() const
    {
        return !m_contents.groupName.isEmpty();
    }

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
    // Actual notification data, whose changes are announced on D-Bus with ready()
    struct Contents {
        QString appName;
        QString ticker;
        QString title;
        QString text;
        QString groupName;
        bool dismissable;
        QString iconPath;
        QString requestReplyId;
        QStringList actions;
        QJsonArray conversation;

        bool operator==(const Contents &other) const = default;
    };

    QString m_internalId;
    Contents m_contents;
    std::optional<Contents> m_lastPublishedContents;
    QString m_payloadHash;
    bool m_hasIcon;
    QPointer<KNotification> m_notification;
    QDir m_imagesDir;
    bool m_silent;
    bool m_ready;
    const Device *m_device;

    struct Message {
        QString sender;
        QString content;

        static Message fromObject(QJsonObject object)
        {
            return {object.value(QStringLiteral("sender")).toString(), object.value(QStringLiteral("content")).toString()};
        }
    };

    void loadIcon(const NetworkPacket &np);
    void iconDownloaded(KJob *job);
    void updateKNotification();

    QString getConversationMessages(bool supportsMarkup) const;

    static QMap<QString, FileTransferJob *> s_downloadsInProgress;
};
