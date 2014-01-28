
#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QDebug>
#include <QCoreApplication>
#include <qdeclarativeexpression.h>
#include <QDeclarativeEngine>

#include "responsewaiter.h"

Q_DECLARE_METATYPE(QDBusPendingReply<>)
Q_DECLARE_METATYPE(QDBusPendingReply<QVariant>)
Q_DECLARE_METATYPE(QDBusPendingReply<bool>)
Q_DECLARE_METATYPE(QDBusPendingReply<int>)
Q_DECLARE_METATYPE(QDBusPendingReply<QString>)

//Q_DECLARE_METATYPE(DBusResponseWaiter::onComplete)
//Q_DECLARE_METATYPE(DBusResponseWaiter::onError)

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
 
    //qRegisterMetaType<DBusResponseWaiter::onComplete>("DBusResponseWaiter::onComplete");
    //qRegisterMetaType<DBusResponseWaiter::onError>("DBusResponseWaiter::onError");
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

void DBusResponse::setPendingCall(QVariant variant)
{
    qDebug() << "spc1";
    m_pendingCall = variant;
    qDebug() << "spc2";
    if (QDBusPendingCall* call = const_cast<QDBusPendingCall*>(DBusResponseWaiter().extractPendingCall(m_pendingCall)))
    {  
        qDebug() << "spc3";
        QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(*call);
        connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(onCallFinished(QDBusPendingCallWatcher*)));
        connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), watcher, SLOT(deleteLater()));
    };
    qDebug() << "spc4";
}


void DBusResponse::onCallFinished(QDBusPendingCallWatcher* watcher)
{
    QVariant variant = watcher->property("pengingCall");
    
    qDebug() << "ocf 1";
    if (QDBusPendingCall* call = const_cast<QDBusPendingCall*>(DBusResponseWaiter().extractPendingCall(m_pendingCall)))
    {
        qDebug() << "ocf 2";
        if (call->isError())
        {
              qDebug() << "ocf 3";
              

//             onError failure = watcher->property("onError").value<onError>();
//              failure(call->error().message());
        }
        else
        {
              qDebug() << "ocf 4444";
    //          onComplete success = watcher->property("onComplete").value<onComplete>();
             
              QDeclarativeExpression *expr = new QDeclarativeExpression(e_->rootContext(), this, "console.debug(this.onError)");
              qDebug() << "ocf 555";
              expr->evaluate();  // result = 400
              qDebug() << "ocf 666";
              
              QDBusMessage reply = call->reply();

              if (reply.arguments().count() > 0)
              {
//                   success(reply.arguments().first());
                
                
                
              }
              else
              {
//                   success(QVariant());
              }
        }
    }
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



