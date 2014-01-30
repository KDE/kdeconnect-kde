
#ifndef RESPONSE_WAITER_H
#define RESPONSE_WAITER_H

#include <QObject>
#include <QVariant>
#include <QDebug>

#include <QDeclarativeEngine>

class QDBusPendingCall;
class QDBusPendingCallWatcher;

class DBusResponse : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(QVariant pendingCall WRITE setPendingCall)
    
public:
    DBusResponse(QObject* parent = 0) : QObject(parent) {}
    virtual ~DBusResponse() {};

    void setPendingCall(QVariant e);
    
Q_SIGNALS:
    void success(QVariant result);
    void error(QString message);
    
private Q_SLOTS:
    void onCallFinished(QDBusPendingCallWatcher* watcher);
};

class DBusResponseWaiter : public QObject
{
    Q_OBJECT
    
public:
  
    DBusResponseWaiter();
    
    virtual ~DBusResponseWaiter(){};
    
    ///extract QDbusPendingCall from \p variant and blocks untill completed
    Q_INVOKABLE QVariant waitForReply(QVariant variant) const;
    
    const QDBusPendingCall* extractPendingCall(QVariant& variant) const;
  
    QList<int> m_registered;
};

#endif
