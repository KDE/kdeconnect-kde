/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "responsewaiter.h"

#include <QCoreApplication>
#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QDebug>

Q_DECLARE_METATYPE(QDBusPendingReply<>)
Q_DECLARE_METATYPE(QDBusPendingReply<QVariant>)
Q_DECLARE_METATYPE(QDBusPendingReply<bool>)
Q_DECLARE_METATYPE(QDBusPendingReply<int>)
Q_DECLARE_METATYPE(QDBusPendingReply<QString>)

DBusResponseWaiter *DBusResponseWaiter::m_instance = nullptr;

DBusResponseWaiter *DBusResponseWaiter::instance()
{
    if (!m_instance) {
        m_instance = new DBusResponseWaiter();
    }
    return m_instance;
}

DBusResponseWaiter::DBusResponseWaiter()
    : QObject()
{
    m_registered << qRegisterMetaType<QDBusPendingReply<>>("QDBusPendingReply<>")
                 << qRegisterMetaType<QDBusPendingReply<QVariant>>("QDBusPendingReply<QVariant>")
                 << qRegisterMetaType<QDBusPendingReply<bool>>("QDBusPendingReply<bool>") << qRegisterMetaType<QDBusPendingReply<int>>("QDBusPendingReply<int>")
                 << qRegisterMetaType<QDBusPendingReply<QString>>("QDBusPendingReply<QString>")
                 << qRegisterMetaType<QDBusPendingReply<QStringList>>("QDBusPendingReply<QStringList>");
}

QVariant DBusResponseWaiter::waitForReply(QVariant variant) const
{
    if (QDBusPendingCall *call = const_cast<QDBusPendingCall *>(extractPendingCall(variant))) {
        call->waitForFinished();

        if (call->isError()) {
            qWarning() << "error:" << call->error();
            return QVariant(QStringLiteral("error"));
        }

        QDBusMessage reply = call->reply();

        if (reply.arguments().count() > 0) {
            return reply.arguments().at(0);
        }
    }
    return QVariant();
}

DBusAsyncResponse::DBusAsyncResponse(QObject *parent)
    : QObject(parent)
    , m_autodelete(false)
{
    m_timeout.setSingleShot(true);
    m_timeout.setInterval(15000);
    connect(&m_timeout, &QTimer::timeout, this, &DBusAsyncResponse::onTimeout);
}

void DBusAsyncResponse::setPendingCall(QVariant variant)
{
    if (QDBusPendingCall *call = const_cast<QDBusPendingCall *>(DBusResponseWaiter::instance()->extractPendingCall(variant))) {
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(*call);
        watcher->setProperty("pendingCallVariant", variant);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, &DBusAsyncResponse::onCallFinished);
        connect(watcher, &QDBusPendingCallWatcher::finished, watcher, &QObject::deleteLater);
        connect(&m_timeout, &QTimer::timeout, watcher, &QObject::deleteLater);
        m_timeout.start();
    } else {
        qWarning() << "error: extractPendingCall didn't work";
    }
}

void DBusAsyncResponse::onCallFinished(QDBusPendingCallWatcher *watcher)
{
    m_timeout.stop();
    QVariant variant = watcher->property("pendingCallVariant");

    if (QDBusPendingCall *call = const_cast<QDBusPendingCall *>(DBusResponseWaiter::instance()->extractPendingCall(variant))) {
        if (call->isError()) {
            Q_EMIT error(call->error().message());
        } else {
            QDBusMessage reply = call->reply();

            if (reply.arguments().count() > 0) {
                Q_EMIT success(reply.arguments().at(0));
            } else {
                Q_EMIT success(QVariant());
            }
        }
    }
    if (m_autodelete) {
        deleteLater();
    }
}

void DBusAsyncResponse::onTimeout()
{
    Q_EMIT error(QStringLiteral("timeout when waiting dbus response!"));
}

const QDBusPendingCall *DBusResponseWaiter::extractPendingCall(QVariant &variant) const
{
    for (int type : std::as_const(m_registered)) {
        if (variant.typeId() == type) {
            return reinterpret_cast<const QDBusPendingCall *>(variant.constData());
        }
    }

    return nullptr;
}

#include "moc_responsewaiter.cpp"
