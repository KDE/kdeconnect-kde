#include "device.h"
#include <ksharedptr.h>
#include <ksharedconfig.h>
#include "devicelinks/devicelink.h"

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
    Q_FOREACH(DeviceLink* existing, m_deviceLinks) {
        //Do not add duplicate links
        if (existing->announcer() == link->announcer()) return;
    }
    m_deviceLinks.append(link);
    connect(link, SIGNAL(receivedPackage(NetworkPackage)), this, SLOT(privateReceivedPackage(NetworkPackage)));
    qSort(m_deviceLinks.begin(),m_deviceLinks.end(),lessThan);
}

void Device::removeLink(DeviceLink* link)
{
    disconnect(link, SIGNAL(receivedPackage(NetworkPackage)), this, SLOT(privateReceivedPackage(NetworkPackage)));
    m_deviceLinks.remove(m_deviceLinks.indexOf(link));
}

void Device::sendPackage(const NetworkPackage& np)
{
    m_deviceLinks.first()->sendPackage(np);
}

void Device::privateReceivedPackage(const NetworkPackage& np)
{
    if (np.type() == "IDENTITY" && !m_knownIdentiy) {
        m_deviceName = np.body();
    } else if (m_paired) {
        qDebug() << "package received from paired device";
        emit receivedPackage(np);
    } else {
        qDebug() << "not paired, ignoring package";
    }
}

void Device::sendPing()
{
    qDebug() << "sendPing";
    NetworkPackage np;
    np.setType("PING");
    sendPackage(np);
}



