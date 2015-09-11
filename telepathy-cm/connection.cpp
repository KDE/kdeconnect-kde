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

#include "connection.h"

#include "textchannel.h"

#include <TelepathyQt/Constants>
#include <TelepathyQt/BaseChannel>
#include <TelepathyQt/Presence>

#include <QDateTime>

#include <QDebug>

Tp::SimpleStatusSpecMap ConnectConnection::getSimpleStatusSpecMap()
{
    //Presence
    Tp::SimpleStatusSpec spAvailable;
    spAvailable.type = Tp::ConnectionPresenceTypeAvailable;
    spAvailable.maySetOnSelf = false;
    spAvailable.canHaveMessage = true;

    Tp::SimpleStatusSpec spOffline;
    spOffline.type = Tp::ConnectionPresenceTypeOffline;
    spOffline.maySetOnSelf = false;
    spOffline.canHaveMessage = false;

    Tp::SimpleStatusSpecMap specs;
    specs.insert(QLatin1String("available"), spAvailable);
    specs.insert(QLatin1String("offline"), spOffline);
    return specs;
}

ConnectConnection::ConnectConnection(const QDBusConnection &dbusConnection, const QString &cmName, const QString &protocolName, const QVariantMap &parameters) :
    Tp::BaseConnection(dbusConnection, cmName, protocolName, parameters)
{
    qDebug() << "making new connection";

    /* Connection.Interface.Contacts */
    contactsIface = Tp::BaseConnectionContactsInterface::create(this);
    contactsIface->setGetContactAttributesCallback(Tp::memFun(this, &ConnectConnection::getContactAttributes));
    contactsIface->setContactAttributeInterfaces(QStringList()
                                                 << TP_QT_IFACE_CONNECTION
                                                 << TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST
                                                 << TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE);
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(contactsIface));

    /* Connection.Interface.SimplePresence */
    simplePresenceIface = Tp::BaseConnectionSimplePresenceInterface::create();
    simplePresenceIface->setSetPresenceCallback(Tp::memFun(this,&ConnectConnection::setPresence));
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(simplePresenceIface));

    /* Connection.Interface.ContactList */
    contactListIface = Tp::BaseConnectionContactListInterface::create();
    contactListIface->setGetContactListAttributesCallback(Tp::memFun(this, &ConnectConnection::getContactListAttributes));
//    contactListIface->setRequestSubscriptionCallback(Tp::memFun(this, &ConnectConnection::requestSubscription));
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(contactListIface));

    /* Connection.Interface.Requests */
    requestsIface = Tp::BaseConnectionRequestsInterface::create(this);
    /* Fill requestableChannelClasses */
    Tp::RequestableChannelClass text;
    text.fixedProperties[TP_QT_IFACE_CHANNEL+".ChannelType"] = TP_QT_IFACE_CHANNEL_TYPE_TEXT;
    text.fixedProperties[TP_QT_IFACE_CHANNEL+".TargetHandleType"]  = Tp::HandleTypeContact;
    text.allowedProperties.append(TP_QT_IFACE_CHANNEL+".TargetHandle");
    text.allowedProperties.append(TP_QT_IFACE_CHANNEL+".TargetID");
    requestsIface->requestableChannelClasses << text;
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(requestsIface));

    QString selfName = QLatin1String("SelfContact");

    if (parameters.contains("self_name")) {
        selfName = parameters.value("self_name").toString();
    }

    if (parameters.contains("device_id")) {
        m_deviceId = parameters.value("device_id").toString();
    }

    setSelfHandle(addContact(selfName + "@kdeconnect_" + m_deviceId));

    setConnectCallback(Tp::memFun(this, &ConnectConnection::connect));
    setInspectHandlesCallback(Tp::memFun(this, &ConnectConnection::inspectHandles));
    setCreateChannelCallback(Tp::memFun(this, &ConnectConnection::createChannelCB));
    setRequestHandlesCallback(Tp::memFun(this, &ConnectConnection::requestHandles));
}

ConnectConnection::~ConnectConnection()
{
    qDebug() << "goodbye connection";
}

void ConnectConnection::connect(Tp::DBusError *error)
{
    setStatus(Tp::ConnectionStatusConnecting, Tp::ConnectionStatusReasonRequested);

    simplePresenceIface->setStatuses(getSimpleStatusSpecMap());

    Tp::SimpleContactPresences presences;
    Tp::SimplePresence presence;
    presence.status = "available";
    presence.statusMessage = "";
    presence.type = Tp::ConnectionPresenceTypeAvailable;
    presences[selfHandle()] = presence;
    simplePresenceIface->setPresences(presences);

    setStatus(Tp::ConnectionStatusConnected, Tp::ConnectionStatusReasonRequested);

    /* Set ContactList status */
    contactListIface->setContactListState(Tp::ContactListStateSuccess);
}

QStringList ConnectConnection::inspectHandles(uint handleType, const Tp::UIntList &handles, Tp::DBusError *error)
{
    qDebug() << Q_FUNC_INFO;

    if (handleType != Tp::HandleTypeContact) {
        error->set(TP_QT_ERROR_INVALID_ARGUMENT, "Unsupported handle type");
        return QStringList();
    }

    QStringList result;

    foreach (uint handle, handles) {
        if (!m_handles.contains(handle)) {
            return QStringList();
        }

        result.append(m_handles.value(handle));
    }

    return result;
}

Tp::BaseChannelPtr ConnectConnection::createChannelCB(const QVariantMap &request, Tp::DBusError *error)
{
    const uint targetHandleType = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")).toUInt();
    const QString channelType = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")).toString();
    
    //note if we ever have this invoked from external clients we need to look for TargetID too and look it up
    const uint targetHandle = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle")).toUInt();

    qDebug() << "ConnectConnection::createChannel " << channelType
             << " " << targetHandleType
             << " " << targetHandle;
 
    if ((targetHandleType != Tp::HandleTypeContact) || (targetHandle == 0)) {
          error->set(TP_QT_ERROR_INVALID_HANDLE, "createChannel error");
          return Tp::BaseChannelPtr();
    }

    Tp::BaseChannelPtr baseChannel = Tp::BaseChannel::create(this, channelType, Tp::HandleType(targetHandleType), targetHandle);

    QString identifier = m_handles.value(targetHandle);

    if (channelType == TP_QT_IFACE_CHANNEL_TYPE_TEXT) {
        ConnectTextChannelPtr textType = ConnectTextChannel::create(this, baseChannel.data(), targetHandle, identifier);
        qDebug() << "Text interface is called " << textType->interfaceName();
        baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(textType));
    }

    return baseChannel;
}

Tp::UIntList ConnectConnection::requestHandles(uint handleType, const QStringList &identifiers, Tp::DBusError *error)
{
    qDebug() << Q_FUNC_INFO << identifiers;

    Tp::UIntList result;

    if (handleType != Tp::HandleTypeContact) {
        error->set(TP_QT_ERROR_INVALID_ARGUMENT, "ConnectConnection::requestHandles - Handle Type unknown");
        return result;
    }

    foreach(const QString &identify, identifiers) {
         uint handle = m_handles.key(identify, 0);
         if (handle) {
             result.append(handle);
         } else {
             result.append(addContact(identify));
         }
     }

    return result;
}

Tp::ContactAttributesMap ConnectConnection::getContactListAttributes(const QStringList &interfaces, bool hold, Tp::DBusError *error)
{
    qDebug() << Q_FUNC_INFO;

    Tp::ContactAttributesMap contactAttributes;

    foreach (const uint handle, m_handles.keys()) {
        if (handle == selfHandle()) {
            continue;
        }
        QVariantMap attributes;
        attributes["org.freedesktop.Telepathy.Connection/contact-id"] = m_handles.value(handle);
        attributes["org.freedesktop.Telepathy.Connection.Interface.ContactList/subscribe"] = Tp::SubscriptionStateYes;
        attributes["org.freedesktop.Telepathy.Connection.Interface.ContactList/publish"] = Tp::SubscriptionStateYes;
        attributes["org.freedesktop.Telepathy.Connection.Interface.ConnectPresence/presence"] = QVariant::fromValue(getPresence(handle));
        contactAttributes[handle] = attributes;
    }
    return contactAttributes;
}

Tp::ContactAttributesMap ConnectConnection::getContactAttributes(const Tp::UIntList &handles, const QStringList &interfaces, Tp::DBusError *error)
{
//    Connection.Interface.Contacts
//    http://telepathy.freedesktop.org/spec/Connection_Interface_Contacts.html#Method:GetContactAttributes
    qDebug() << Q_FUNC_INFO << handles;

    Tp::ContactAttributesMap contactAttributes;

    foreach (const uint handle, handles) {
        if (m_handles.contains(handle)){
            QVariantMap attributes;
            attributes["org.freedesktop.Telepathy.Connection/contact-id"] = m_handles.value(handle);

            if (handle != selfHandle() && interfaces.contains("org.freedesktop.Telepathy.Connection.Interface.ContactList")) {
                attributes["org.freedesktop.Telepathy.Connection.Interface.ContactList/subscribe"] = Tp::SubscriptionStateYes;
                attributes["org.freedesktop.Telepathy.Connection.Interface.ContactList/publish"] = Tp::SubscriptionStateYes;
                attributes["org.freedesktop.Telepathy.Connection.Interface.SimplePresence/presence"] = QVariant::fromValue(getPresence(handle));
            }
            contactAttributes[handle] = attributes;
        }
    }
    return contactAttributes;
}

Tp::SimplePresence ConnectConnection::getPresence(uint handle)
{
    return Tp::Presence::offline().barePresence();
}

uint ConnectConnection::setPresence(const QString &status, const QString &message, Tp::DBusError *error)
{
    qDebug() << Q_FUNC_INFO << "not implemented";
    return 0;
}

uint ConnectConnection::ensureContact(const QString &identifier)
{
    uint handle = getHandle(identifier);
    if (!handle) {
        handle = addContact(identifier);
    }
    return handle;
}

uint ConnectConnection::addContacts(const QStringList &identifiers)
{
    qDebug() << Q_FUNC_INFO;
    uint handle = 0;

    if (!m_handles.isEmpty()) {
        handle = m_handles.keys().last();
    }

    QList<uint> newHandles;
    foreach(const QString &identifier, identifiers) {
        ++handle;
        m_handles.insert(handle, identifier);
        newHandles << handle;
    }

    return handle;
}

uint ConnectConnection::addContact(const QString &identifier)
{
    qDebug() << Q_FUNC_INFO;
    return addContacts(QStringList() << identifier);
}

/* Receive message from someone to ourself */
bool ConnectConnection::receiveMessage(const QString &sender, const QString &message)
{
    uint senderHandle, targetHandle;

    Tp::HandleType handleType = Tp::HandleTypeContact;
    senderHandle = targetHandle = ensureContact(sender);

    Tp::DBusError error;
    bool yours;
    
    QVariantMap request;
    request[TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")] = TP_QT_IFACE_CHANNEL_TYPE_TEXT;
    request[TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle")] = targetHandle;
    request[TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")] = Tp::HandleTypeContact;
    request[TP_QT_IFACE_CHANNEL + QLatin1String(".InitiatorHandle")] = targetHandle; //they texted you, so they started it
    Tp::BaseChannelPtr channel = ensureChannel(request, yours, false, &error);
    if (error.isValid()) {
        qWarning() << "ensureChannel failed:" << error.name() << " " << error.message();
        return false;
    }

    Tp::BaseChannelTextTypePtr textChannel = Tp::BaseChannelTextTypePtr::dynamicCast(channel->interface(TP_QT_IFACE_CHANNEL_TYPE_TEXT));
    if (!textChannel) {
        qDebug() << "Error, channel is not a textChannel??";
        return false;
    }

    uint timestamp = QDateTime::currentMSecsSinceEpoch() / 1000;

    Tp::MessagePartList body;
    Tp::MessagePart text;
    text["content-type"]            = QDBusVariant("text/plain");
    text["content"]                 = QDBusVariant(message);
    body << text;

    Tp::MessagePartList partList;
    Tp::MessagePart header;
    header["message-received"]      = QDBusVariant(timestamp);
    header["message-sender"]        = QDBusVariant(senderHandle);
    header["message-sender-id"]     = QDBusVariant(sender);
    //header["sender-nickname"]       = QDBusVariant(pushName);
    header["message-type"]          = QDBusVariant(Tp::ChannelTextMessageTypeNormal);

    partList << header << body;
    textChannel->addReceivedMessage(partList);
    return true;
}

void ConnectConnection::setContactList(const QStringList &identifiers)
{
    // Actually it don't clear previous list (not implemented yet)
    addContacts(identifiers);

//    Tp::ContactSubscriptionMap changes;
//    Tp::HandleIdentifierMap identifiers;
//    Tp::HandleIdentifierMap removals;

    QList<uint> handles;

    for (int i = 0; i < identifiers.count(); ++i) {
        handles.append(ensureContact(identifiers.at(i)));
    }
}

uint ConnectConnection::getHandle(const QString &identifier) const
{
    foreach (uint key, m_handles.keys()) {
        if (m_handles.value(key) == identifier) {
            return key;
        }
    }

    return 0;
}
