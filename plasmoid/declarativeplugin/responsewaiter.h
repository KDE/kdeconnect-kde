
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
