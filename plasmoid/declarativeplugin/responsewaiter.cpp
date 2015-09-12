#include "responsewaiter.h"

#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QDebug>
#include <QCoreApplication>

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
            return QVariant("error");
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
    connect(&m_timeout, SIGNAL(timeout()), this, SLOT(onTimeout()));
}


void DBusAsyncResponse::setPendingCall(QVariant variant)
{
    if (QDBusPendingCall* call = const_cast<QDBusPendingCall*>(DBusResponseWaiter::instance()->extractPendingCall(variant)))
    {  
        QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(*call);
        watcher->setProperty("pengingCallVariant", variant);
        connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(onCallFinished(QDBusPendingCallWatcher*)));
        connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), watcher, SLOT(deleteLater()));
        connect(&m_timeout, SIGNAL(timeout()), watcher, SLOT(deleteLater()));
        m_timeout.start();
    };
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
    Q_EMIT error("timeout when waiting dbus response!");
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
    
    return nullptr;
}



