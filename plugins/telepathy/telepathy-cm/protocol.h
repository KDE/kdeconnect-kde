/*
    Copyright (C) 2014 Alexandr Akulich <akulichalexander@gmail.com>

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
    LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
    OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef CONNECTCM_PROTOCOL_H
#define CONNECTCM_PROTOCOL_H

#include <TelepathyQt/BaseProtocol>

class ConnectConnection;

class KDEConnectTelepathyProtocol : public Tp::BaseProtocol
{
    Q_OBJECT
    Q_DISABLE_COPY(KDEConnectTelepathyProtocol)
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Telepathy.ConnectionManager.kdeconnect")

public:
    KDEConnectTelepathyProtocol(const QDBusConnection &dbusConnection, const QString &name);
    ~KDEConnectTelepathyProtocol() override;

    QString connectionManagerName() const;
    void setConnectionManagerName(const QString &newName);

public Q_SLOTS:
    bool sendMessage(QString sender, QString message);
    void setContactList(QStringList list);

Q_SIGNALS:
    void contactsListChanged(QStringList);
    void messageReceived(QString sender, QString message);

private:
    Tp::BaseConnectionPtr createConnection(const QVariantMap &parameters, Tp::DBusError *error);
    QString identifyAccount(const QVariantMap &parameters, Tp::DBusError *error);
    QString normalizeContact(const QString &contactId, Tp::DBusError *error);

    // Proto.I.Addressing
    QString normalizeVCardAddress(const QString &vCardField, const QString vCardAddress,
            Tp::DBusError *error);
    QString normalizeContactUri(const QString &uri, Tp::DBusError *error);

    Tp::BaseProtocolAddressingInterfacePtr addrIface;
    Tp::BaseProtocolAvatarsInterfacePtr avatarsIface;

    QString m_connectionManagerName;
    
    //normally keeping the connection in the protocol would be really weird
    //however we want to proxy the messages to the active connection and want a single entry point
    Tp::SharedPtr<ConnectConnection> m_connection; 
};

inline QString KDEConnectTelepathyProtocol::connectionManagerName() const
{
    return m_connectionManagerName;
}

#endif // CONNECTCM_PROTOCOL_H
