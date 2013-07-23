#include "device.h"
#include <ksharedptr.h>
#include <ksharedconfig.h>
#include "devicelinks/devicelink.h"
#include "announcers/announcer.h"

#include <KConfigGroup>
#include <QDebug>

Device::Device(const QString& id, const QString& name)
{
    m_deviceId = id;
    m_deviceName = name;
    m_paired = true;
    m_knownIdentiy = true;
    QDBusConnection::sessionBus().registerObject("/modules/kdeconnect/Devices/"+id, this, QDBusConnection::ExportScriptableContents);
}

Device::Device(const QString& id, const QString& name, DeviceLink* link)
{
    m_deviceId = id;
    m_deviceName = name;
    m_paired = false;
    m_knownIdentiy = true;
    QDBusConnection::sessionBus().registerObject("/modules/kdeconnect/Devices/"+id, this, QDBusConnection::ExportScriptableContents);

    addLink(link);
}
/*
Device::Device(const QString& id, const QString& name, DeviceLink* link)
{
    m_deviceId = id;
    m_deviceName = id; //Temporary name
    m_paired = false;
    m_knownIdentiy = false;

    addLink(link);

    NetworkPackage identityRequest;
    identityRequest.setType("IDENTITY_REQUEST");
    link->sendPackage(identityRequest);

    QDBusConnection::sessionBus().registerObject("/modules/kdeconnect/Devices/"+id, this);
}
*/
void Device::setPair(bool b)
{
    qDebug() << "setPair" << b;
    m_paired = b;
    KSharedConfigPtr config = KSharedConfig::openConfig("kdeconnectrc");
    if (b) {
        qDebug() << name() << "paired";
        config->group("devices").group("paired").group(id()).writeEntry("name",name());
    } else {
        qDebug() << name() << "unpaired";
        config->group("devices").group("paired").deleteGroup(id());
    }
}

static bool lessThan(DeviceLink* p1, DeviceLink* p2)
{
    return p1->announcer()->priority() > p2->announcer()->priority();
}

void Device::addLink(DeviceLink* link)
{
    qDebug() << "Adding link to " << id() << "via" << link->announcer();

    connect(link,SIGNAL(destroyed(QObject*)),this,SLOT(linkDestroyed(QObject*)));

    m_deviceLinks.append(link);
    connect(link, SIGNAL(receivedPackage(NetworkPackage)), this, SLOT(privateReceivedPackage(NetworkPackage)));

    qSort(m_deviceLinks.begin(),m_deviceLinks.end(),lessThan);
}

void Device::linkDestroyed(QObject* o)
{
    qDebug() << "Link destroyed";
    removeLink(static_cast<DeviceLink*>(o));
}

void Device::removeLink(DeviceLink* link)
{
    qDebug() << "RemoveLink";
    //disconnect(link, SIGNAL(receivedPackage(NetworkPackage)), this, SLOT(privateReceivedPackage(NetworkPackage)));
    m_deviceLinks.removeOne(link);
}

bool Device::sendPackage(const NetworkPackage& np)
{
    Q_FOREACH(DeviceLink* dl, m_deviceLinks) {
        if (dl->sendPackage(np)) return true;
    }
    return false;
}

void Device::privateReceivedPackage(const NetworkPackage& np)
{
    if (np.type() == "kdeconnect.identity" && !m_knownIdentiy) {
        m_deviceName = np.get<QString>("deviceName");
    } else if (m_paired) {
        qDebug() << "package received from paired device";
        emit receivedPackage(*this, np);
    } else {
        qDebug() << "not paired, ignoring package";
    }
}

QStringList Device::availableLinks() const
{
    QStringList sl;
    Q_FOREACH(DeviceLink* dl, m_deviceLinks) {
        sl.append(dl->announcer()->name());
    }
    return sl;
}

void Device::sendPing()
{
    NetworkPackage np("kdeconnect.ping");
    bool success = sendPackage(np);
    qDebug() << "sendPing:" << success;
}



