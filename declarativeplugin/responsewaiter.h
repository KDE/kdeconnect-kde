/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef RESPONSE_WAITER_H
#define RESPONSE_WAITER_H

#include <QObject>
#include <QTimer>
#include <QVariant>

class QDBusPendingCall;
class QDBusPendingCallWatcher;

class DBusResponseWaiter : public QObject
{
    Q_OBJECT

public:
    static DBusResponseWaiter *instance();

    /// extract QDbusPendingCall from \p variant and blocks until completed
    Q_INVOKABLE QVariant waitForReply(QVariant variant) const;

    const QDBusPendingCall *extractPendingCall(QVariant &variant) const;

private:
    DBusResponseWaiter();

    static DBusResponseWaiter *m_instance;
    QList<int> m_registered;
};

class DBusAsyncResponse : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool autoDelete READ autodelete WRITE setAutodelete)

public:
    explicit DBusAsyncResponse(QObject *parent = nullptr);

    Q_INVOKABLE void setPendingCall(QVariant e);

    void setAutodelete(bool b)
    {
        m_autodelete = b;
    };
    bool autodelete() const
    {
        return m_autodelete;
    }

Q_SIGNALS:
    void success(const QVariant &result);
    void error(const QString &message);

private Q_SLOTS:
    void onCallFinished(QDBusPendingCallWatcher *watcher);
    void onTimeout();

private:
    QTimer m_timeout;
    bool m_autodelete;
};

#endif
