
#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QDebug>
#include <QCoreApplication>
#include <qdeclarativeexpression.h>
#include <QDeclarativeEngine>
#include <QDeclarativeContext>

#include "responsewaiter.h"

Q_DECLARE_METATYPE(QDBusPendingReply<>)
Q_DECLARE_METATYPE(QDBusPendingReply<QVariant>)
Q_DECLARE_METATYPE(QDBusPendingReply<bool>)
Q_DECLARE_METATYPE(QDBusPendingReply<int>)
Q_DECLARE_METATYPE(QDBusPendingReply<QString>)

DBusResponseWaiter* DBusResponseWaiter::m_instance = 0;

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
        QDBusMessage reply = call->reply();

        if (reply.arguments().count() > 0)
        {
            return reply.arguments().first();
        }
    }
    return QVariant();
}

void DBusAsyncResponse::setPendingCall(QVariant variant)
{
    if (QDBusPendingCall* call = const_cast<QDBusPendingCall*>(DBusResponseWaiter::instance()->extractPendingCall(variant)))
    {  
        QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(*call);
        watcher->setProperty("pengingCall", variant);
        connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(onCallFinished(QDBusPendingCallWatcher*)));
        connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), watcher, SLOT(deleteLater()));
    };
}


void DBusAsyncResponse::onCallFinished(QDBusPendingCallWatcher* watcher)
{
    QVariant variant = watcher->property("pengingCall");
    
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
                  Q_EMIT success(reply.arguments().first());
              }
              else
              {
                  Q_EMIT success(QVariant());
              }
        }
    }
    deleteLater();
}

const QDBusPendingCall* DBusResponseWaiter::extractPendingCall(QVariant& variant) const
{
    Q_FOREACH(int type, m_registered) 
    {
        if (variant.canConvert(QVariant::Type(type)))
        {
            return reinterpret_cast<const QDBusPendingCall*>(variant.constData());  
        }
    }
    
    return 0;
}



