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

#include "responsewaiter.h"

#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QDebug>
#include <QCoreApplication>

//In older Qt released, qAsConst isnt available
#include "core/qtcompat_p.h"

Q_DECLARE_METATYPE(QDBusPendingReply<>)
Q_DECLARE_METATYPE(QDBusPendingReply<QVariant>)
Q_DECLARE_METATYPE(QDBusPendingReply<bool>)
Q_DECLARE_METATYPE(QDBusPendingReply<int>)
Q_DECLARE_METATYPE(QDBusPendingReply<QString>)

DBusResponseWaiter* DBusResponseWaiter::m_instance = nullptr;

DBusResponseWaiter* DBusResponseWaiter::instance()
{
    if (!m_instance)
    {
        m_instance = new DBusResponseWaiter();
    }
    return m_instance;
}

DBusResponseWaiter::DBusResponseWaiter()
    : QObject()
{
    m_registered
        << qRegisterMetaType<QDBusPendingReply<> >("QDBusPendingReply<>")
        << qRegisterMetaType<QDBusPendingReply<QVariant> >("QDBusPendingReply<QVariant>")
        << qRegisterMetaType<QDBusPendingReply<bool> >("QDBusPendingReply<bool>")
        << qRegisterMetaType<QDBusPendingReply<int> >("QDBusPendingReply<int>")
        << qRegisterMetaType<QDBusPendingReply<QString> >("QDBusPendingReply<QString>")
    ;
}

QVariant DBusResponseWaiter::waitForReply(QVariant variant) const
{
    if (QDBusPendingCall* call = const_cast<QDBusPendingCall*>(extractPendingCall(variant)))
    {
        call->waitForFinished();

        if (call->isError())
        {
            qWarning() << "error:" << call->error();
            return QVariant(QStringLiteral("error"));
        }

        QDBusMessage reply = call->reply();

        if (reply.arguments().count() > 0)
        {
            return reply.arguments().at(0);
        }
    }
    return QVariant();
}

DBusAsyncResponse::DBusAsyncResponse(QObject* parent)
    : QObject(parent)
    , m_autodelete(false)
{
    m_timeout.setSingleShot(true);
    m_timeout.setInterval(15000);
    connect(&m_timeout, &QTimer::timeout, this, &DBusAsyncResponse::onTimeout);
}


void DBusAsyncResponse::setPendingCall(QVariant variant)
{
    if (QDBusPendingCall* call = const_cast<QDBusPendingCall*>(DBusResponseWaiter::instance()->extractPendingCall(variant)))
    {
        QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(*call);
        watcher->setProperty("pengingCallVariant", variant);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, &DBusAsyncResponse::onCallFinished);
        connect(watcher, &QDBusPendingCallWatcher::finished, watcher, &QObject::deleteLater);
        connect(&m_timeout, &QTimer::timeout, watcher, &QObject::deleteLater);
        m_timeout.start();
    }
}

void DBusAsyncResponse::onCallFinished(QDBusPendingCallWatcher* watcher)
{
    m_timeout.stop();
    QVariant variant = watcher->property("pengingCallVariant");

    if (QDBusPendingCall* call = const_cast<QDBusPendingCall*>(DBusResponseWaiter::instance()->extractPendingCall(variant)))
    {
        if (call->isError())
        {
            Q_EMIT error(call->error().message());
        }
        else
        {
              QDBusMessage reply = call->reply();

              if (reply.arguments().count() > 0)
              {
                  Q_EMIT success(reply.arguments().at(0));
              }
              else
              {
                  Q_EMIT success(QVariant());
              }
        }
    }
    if (m_autodelete)
    {
        deleteLater();
    }
}

void DBusAsyncResponse::onTimeout()
{
    Q_EMIT error(QStringLiteral("timeout when waiting dbus response!"));
}

const QDBusPendingCall* DBusResponseWaiter::extractPendingCall(QVariant& variant) const
{
    for (int type : qAsConst(m_registered))
    {
        if (variant.canConvert(QVariant::Type(type)))
        {
            return reinterpret_cast<const QDBusPendingCall*>(variant.constData());
        }
    }

    return nullptr;
}



