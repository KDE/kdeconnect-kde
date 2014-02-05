
#ifndef RESPONSE_WAITER_H
#define RESPONSE_WAITER_H

#include <QObject>
#include <QVariant>
#include <QDebug>

#include <QDeclarativeEngine>

class QDBusPendingCall;
class QDBusPendingCallWatcher;

class DBusResponseWaiter : public QObject
{
    Q_OBJECT
    
public:
  
    static DBusResponseWaiter* instance();
  
    ///extract QDbusPendingCall from \p variant and blocks untill completed
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
    
    Q_PROPERTY(QVariant pendingCall WRITE setPendingCall)
    Q_PROPERTY(bool autoDelete WRITE setAutodelete)
    
public:
    DBusAsyncResponse(QObject* parent = 0) : QObject(parent), m_autodelete(false) {}
    virtual ~DBusAsyncResponse() {}; 

    void setPendingCall(QVariant e);
    void setAutodelete(bool b) {m_autodelete = b;};
    
Q_SIGNALS:
    void success(QVariant result);
    void error(QString message);
    
private Q_SLOTS:
    void onCallFinished(QDBusPendingCallWatcher* watcher);
    
private:    
    bool m_autodelete;
};


#endif
