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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef RESPONSE_WAITER_H
#define RESPONSE_WAITER_H

#include <QObject>
#include <QVariant>
#include <QTimer>

class QDBusPendingCall;
class QDBusPendingCallWatcher;

class DBusResponseWaiter : public QObject
{
    Q_OBJECT

public:

    static DBusResponseWaiter* instance();

    ///extract QDbusPendingCall from \p variant and blocks until completed
    Q_INVOKABLE QVariant waitForReply(QVariant variant) const;

    const QDBusPendingCall* extractPendingCall(QVariant& variant) const;

private:
    DBusResponseWaiter();

    static DBusResponseWaiter* m_instance;
    QList<int> m_registered;
};


class DBusAsyncResponse : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool autoDelete READ autodelete WRITE setAutodelete)

public:
    explicit DBusAsyncResponse(QObject* parent = nullptr);
    ~DBusAsyncResponse() override = default;

    Q_INVOKABLE void setPendingCall(QVariant e);

    void setAutodelete(bool b) {m_autodelete = b;};
    bool autodelete() const {return m_autodelete;}

Q_SIGNALS:
    void success(const QVariant& result);
    void error(const QString& message);

private Q_SLOTS:
    void onCallFinished(QDBusPendingCallWatcher* watcher);
    void onTimeout();

private:
    QTimer m_timeout;
    bool m_autodelete;
};


#endif
