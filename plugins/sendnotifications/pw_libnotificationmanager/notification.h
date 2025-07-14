/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QDateTime>
#include <QImage>
#include <QList>
#include <QString>
#include <QUrl>

namespace NotificationManager
{
/**
 * @short Represents a single notification.
 *
 * @author Kai Uwe Broulik <kde@privat.broulik.de>
 **/
class Notification
{
public:
    explicit Notification(uint id = 0);

    Notification(const Notification &other);
    Notification(Notification &&other) Q_DECL_NOEXCEPT;

    Notification &operator=(const Notification &other);
    Notification &operator=(Notification &&other) Q_DECL_NOEXCEPT;

    virtual ~Notification();

    uint id() const;

    QString dBusService() const;
    void setDBusService(const QString &dBusService);

    // Creation time of the original notification
    QDateTime created() const;
    void setCreated(const QDateTime &created);

    // Last time it was updated, or invalid if it never was
    QDateTime updated() const;
    void resetUpdated();

    bool read() const;
    void setRead(bool read);

    QString summary() const;
    void setSummary(const QString &summary);

    QString body() const;
    void setBody(const QString &body);

    // This returns the raw body data as provided by the notification
    // this is useful when you want to html sanitization at different
    // stage then the notification server.
    QString rawBody() const;

    QString icon() const;
    void setIcon(const QString &icon);

    QImage image() const;
    void setImage(const QImage &image);

    QString desktopEntry() const;
    void setDesktopEntry(const QString &desktopEntry);

    QString notifyRcName() const;
    QString eventId() const;

    QString applicationName() const;
    void setApplicationName(const QString &applicationName);

    QString applicationIconName() const;
    void setApplicationIconName(const QString &applicationIconName);

    QString originName() const;

    // should we group the two into a QPair or something?
    QStringList actionNames() const;
    QStringList actionLabels() const;
    bool hasDefaultAction() const;
    QString defaultActionLabel() const;
    void setActions(const QStringList &actions);

    QList<QUrl> urls() const;
    void setUrls(const QList<QUrl> &urls);

    // FIXME use separate enum again
    int urgency() const;
    void setUrgency(int urgency);

    bool userActionFeedback() const;

    int timeout() const;
    void setTimeout(int timeout);

    bool configurable() const;
    QString configureActionLabel() const;

    bool hasReplyAction() const;
    QString replyActionLabel() const;
    QString replyPlaceholderText() const;
    QString replySubmitButtonText() const;
    QString replySubmitButtonIconName() const;

    QString category() const;

    bool expired() const;
    void setExpired(bool expired);

    bool dismissed() const;
    void setDismissed(bool dismissed);

    bool resident() const;
    void setResident(bool resident);

    bool transient() const;
    void setTransient(bool transient);

    // Little bit of mess here, we want to sometime keep track of processed hints, and not process it.
    QVariantMap hints() const;
    void setHints(const QVariantMap &hints);

    void processHints(const QVariantMap &hints);

    bool wasAddedDuringInhibition() const;
    void setWasAddedDuringInhibition(bool value);

private:
    friend class NotificationsModel;
    friend class AbstractNotificationsModel;
    friend class ServerPrivate;

    class Private;
    Private *d;
};

} // namespace NotificationManager
