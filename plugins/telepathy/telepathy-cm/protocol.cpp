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

#include "protocol.h"
#include "connection.h"

#include <TelepathyQt/BaseConnection>
#include <TelepathyQt/Constants>
#include <TelepathyQt/RequestableChannelClassSpec>
#include <TelepathyQt/RequestableChannelClassSpecList>
#include <TelepathyQt/Types>

#include <QLatin1String>
#include <QVariantMap>

KDEConnectTelepathyProtocol::KDEConnectTelepathyProtocol(const QDBusConnection &dbusConnection, const QString &name)
    : BaseProtocol(dbusConnection, name)
{
//     setParameters(Tp::ProtocolParameterList()
//                   << Tp::ProtocolParameter(QLatin1String("device_id"), QLatin1String("s"), Tp::ConnMgrParamFlagRequired)
//                   << Tp::ProtocolParameter(QLatin1String("self_name"), QLatin1String("s"), 0));

    setRequestableChannelClasses(Tp::RequestableChannelClassSpecList() << Tp::RequestableChannelClassSpec::textChat());

    // callbacks
    setCreateConnectionCallback(memFun(this, &KDEConnectTelepathyProtocol::createConnection));
    setIdentifyAccountCallback(memFun(this, &KDEConnectTelepathyProtocol::identifyAccount));
    setNormalizeContactCallback(memFun(this, &KDEConnectTelepathyProtocol::normalizeContact));

    addrIface = Tp::BaseProtocolAddressingInterface::create();
    addrIface->setAddressableVCardFields(QStringList() << QLatin1String("x-example-vcard-field"));
    addrIface->setAddressableUriSchemes(QStringList() << QLatin1String("example-uri-scheme"));
    addrIface->setNormalizeVCardAddressCallback(memFun(this, &KDEConnectTelepathyProtocol::normalizeVCardAddress));
    addrIface->setNormalizeContactUriCallback(memFun(this, &KDEConnectTelepathyProtocol::normalizeContactUri));
    plugInterface(Tp::AbstractProtocolInterfacePtr::dynamicCast(addrIface));
/*
    presenceIface = Tp::BaseProtocolPresenceInterface::create();
    presenceIface->setStatuses(Tp::PresenceSpecList(ConnectConnection::getConnectStatusSpecMap()));
    plugInterface(Tp::AbstractProtocolInterfacePtr::dynamicCast(presenceIface));*/

    auto bus = QDBusConnection::sessionBus();
    bus.registerObject("/kdeconnect", this, QDBusConnection::ExportAllSignals | QDBusConnection::ExportAllSlots);

    Tp::DBusError err;
}

KDEConnectTelepathyProtocol::~KDEConnectTelepathyProtocol()
{
}

void KDEConnectTelepathyProtocol::setConnectionManagerName(const QString &newName)
{
    m_connectionManagerName = newName;
}

bool KDEConnectTelepathyProtocol::sendMessage(QString sender, QString message)
{
    if (m_connection) {
        return m_connection->receiveMessage(sender, message);
    }
    return false;
}

void KDEConnectTelepathyProtocol::setContactList(QStringList list)
{
    if (m_connection) {
        m_connection->setContactList(list);
    }
}

Tp::BaseConnectionPtr KDEConnectTelepathyProtocol::createConnection(const QVariantMap &parameters, Tp::DBusError *error)
{
    Q_UNUSED(error)

    auto newConnection = Tp::BaseConnection::create<ConnectConnection>(m_connectionManagerName, this->name(), parameters);
    connect(newConnection.constData(), SIGNAL(messageReceived(QString,QString)), SIGNAL(messageReceived(QString,QString)));
    m_connection = newConnection;

    return newConnection;
}

QString KDEConnectTelepathyProtocol::identifyAccount(const QVariantMap &parameters, Tp::DBusError *error)
{
    qDebug() << Q_FUNC_INFO << parameters;
    error->set(QLatin1String("IdentifyAccount.Error.NotImplemented"), QLatin1String(""));
    return QString();
}

QString KDEConnectTelepathyProtocol::normalizeContact(const QString &contactId, Tp::DBusError *error)
{
    qDebug() << Q_FUNC_INFO << contactId;
    error->set(QLatin1String("NormalizeContact.Error.NotImplemented"), QLatin1String(""));
    return QString();
}

QString KDEConnectTelepathyProtocol::normalizeVCardAddress(const QString &vcardField, const QString vcardAddress,
        Tp::DBusError *error)
{
    qDebug() << Q_FUNC_INFO << vcardField << vcardAddress;
    error->set(QLatin1String("NormalizeVCardAddress.Error.NotImplemented"), QLatin1String(""));
    return QString();
}

QString KDEConnectTelepathyProtocol::normalizeContactUri(const QString &uri, Tp::DBusError *error)
{
    qDebug() << Q_FUNC_INFO << uri;
    error->set(QLatin1String("NormalizeContactUri.Error.NotImplemented"), QLatin1String(""));
    return QString();
}
