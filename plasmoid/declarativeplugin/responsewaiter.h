
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
    
    Q_PROPERTY(QVariant pendingCall READ pendingCall WRITE setPendingCall NOTIFY pendingCallChanged)
    Q_PROPERTY(QVariant onError READ onError WRITE setOnError NOTIFY onErrorChanged)
    Q_PROPERTY(QVariant onSuccess READ onSuccess WRITE setOnSuccess NOTIFY onSuccessChanged)
    
public:
    DBusResponse(QDeclarativeEngine* e = 0) : QObject(e) , e_(e) {qDebug() << "C";};
    virtual ~DBusResponse() {};

    void setPendingCall(QVariant e);
    QVariant pendingCall() {return m_pendingCall;}
    
    
    void setOnError(QVariant e) {m_onError = e;}
    QVariant onError() {return m_onError;}
    
    void setOnSuccess(QVariant e) {m_onSuccess = e;}
    QVariant onSuccess() {return m_onSuccess;}

Q_SIGNALS:
    void onSuccessChanged();
    void onErrorChanged();
    void pendingCallChanged();
    
private Q_SLOTS:
    void onCallFinished(QDBusPendingCallWatcher* watcher);
   
private:
  QVariant m_pendingCall;
  QVariant m_onError;
  QVariant m_onSuccess;
  
  QDeclarativeEngine* e_;
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
