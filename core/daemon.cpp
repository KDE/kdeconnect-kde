/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "daemon.h"

#include <QDBusMetaType>
#include <QNetworkAccessManager>
#include <QDebug>
#include <QPointer>
#include <QProcess>

#include "core_debug.h"
#include "kdeconnectconfig.h"
#include "networkpacket.h"
#include "dbushelper.h"
#include "notificationserverinfo.h"

#ifdef KDECONNECT_BLUETOOTH
    #include "backends/bluetooth/bluetoothlinkprovider.h"
#endif

#include "backends/lan/lanlinkprovider.h"
#include "backends/loopback/loopbacklinkprovider.h"
#include "device.h"
#include "backends/devicelink.h"
#include "backends/linkprovider.h"

//In older Qt released, qAsConst isnt available
#include "qtcompat_p.h"

static Daemon* s_instance = nullptr;

struct DaemonPrivate
{
    //Different ways to find devices and connect to them
    QSet<LinkProvider*> m_linkProviders;

    //Every known device
    QMap<QString, Device*> m_devices;

    QSet<QString> m_discoveryModeAcquisitions;
    bool m_testMode;
};

Daemon* Daemon::instance()
{
    Q_ASSERT(s_instance != nullptr);
    return s_instance;
}

Daemon::Daemon(QObject* parent, bool testMode)
    : QObject(parent)
    , d(new DaemonPrivate)
{
    Q_ASSERT(!s_instance);
    s_instance = this;
    d->m_testMode = testMode;

    // HACK init may call pure virtual functions from this class so it can't be called directly from the ctor
    QTimer::singleShot(0, this, &Daemon::init);
}

void Daemon::init()
{
    qCDebug(KDECONNECT_CORE) << "Daemon starting";

    //Load backends
    if (d->m_testMode)
        d->m_linkProviders.insert(new LoopbackLinkProvider());
    else {
        d->m_linkProviders.insert(new LanLinkProvider());
        #ifdef KDECONNECT_BLUETOOTH
            d->m_linkProviders.insert(new BluetoothLinkProvider());
        #endif
        #ifdef KDECONNECT_LOOPBACK
            d->m_linkProviders.insert(new LoopbackLinkProvider());
        #endif
    }

    //Register on DBus
    qDBusRegisterMetaType< QMap<QString,QString> >();
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.kdeconnect"));
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/modules/kdeconnect"), this, QDBusConnection::ExportScriptableContents);

    //Read remembered paired devices
    const QStringList& list = KdeConnectConfig::instance().trustedDevices();
    for (const QString& id : list) {
        addDevice(new Device(this, id));
    }

    //Listen to new devices
    for (LinkProvider* a : qAsConst(d->m_linkProviders)) {
        connect(a, &LinkProvider::onConnectionReceived,
                this, &Daemon::onNewDeviceLink);
        a->onStart();
    }

    NotificationServerInfo::instance().init();

    qCDebug(KDECONNECT_CORE) << "Daemon started";
}

void Daemon::acquireDiscoveryMode(const QString& key)
{
    bool oldState = d->m_discoveryModeAcquisitions.isEmpty();

    d->m_discoveryModeAcquisitions.insert(key);

    if (oldState != d->m_discoveryModeAcquisitions.isEmpty()) {
        forceOnNetworkChange();
    }
}

void Daemon::releaseDiscoveryMode(const QString& key)
{
    bool oldState = d->m_discoveryModeAcquisitions.isEmpty();

    d->m_discoveryModeAcquisitions.remove(key);

    if (oldState != d->m_discoveryModeAcquisitions.isEmpty()) {
        cleanDevices();
    }
}

void Daemon::removeDevice(Device* device)
{
    d->m_devices.remove(device->id());
    device->deleteLater();
    Q_EMIT deviceRemoved(device->id());
    Q_EMIT deviceListChanged();
}

void Daemon::cleanDevices()
{
    const auto devs = d->m_devices;
    for (Device* device : devs) {
        if (device->isTrusted()) {
            continue;
        }
        device->cleanUnneededLinks();
        //If there are no links remaining
        if (!device->isReachable()) {
            removeDevice(device);
        }
    }
}

void Daemon::forceOnNetworkChange()
{
    qCDebug(KDECONNECT_CORE) << "Sending onNetworkChange to" << d->m_linkProviders.size() << "LinkProviders";
    for (LinkProvider* a : qAsConst(d->m_linkProviders)) {
        a->onNetworkChange();
    }
}

Device* Daemon::getDevice(const QString& deviceId)
{
    for (Device* device : qAsConst(d->m_devices)) {
        if (device->id() == deviceId) {
            return device;
        }
    }
    return nullptr;
}

const QSet<LinkProvider*>& Daemon::getLinkProviders() const
{
    return d->m_linkProviders;
}

QStringList Daemon::devices(bool onlyReachable, bool onlyTrusted) const
{
    QStringList ret;
    for (Device* device : qAsConst(d->m_devices)) {
        if (onlyReachable && !device->isReachable()) continue;
        if (onlyTrusted && !device->isTrusted()) continue;
        ret.append(device->id());
    }
    return ret;
}

QMap<QString, QString> Daemon::deviceNames(bool onlyReachable, bool onlyTrusted) const
{
    QMap<QString, QString> ret;
    for (Device* device : qAsConst(d->m_devices)) {
        if (onlyReachable && !device->isReachable()) continue;
        if (onlyTrusted && !device->isTrusted()) continue;
        ret[device->id()] = device->name();
    }
    return ret;
}

void Daemon::onNewDeviceLink(const NetworkPacket& identityPacket, DeviceLink* dl)
{
    const QString& id = identityPacket.get<QString>(QStringLiteral("deviceId"));

    //qCDebug(KDECONNECT_CORE) << "Device discovered" << id << "via" << dl->provider()->name();

    if (d->m_devices.contains(id)) {
        qCDebug(KDECONNECT_CORE) << "It is a known device" << identityPacket.get<QString>(QStringLiteral("deviceName"));
        Device* device = d->m_devices[id];
        bool wasReachable = device->isReachable();
        device->addLink(identityPacket, dl);
        if (!wasReachable) {
            Q_EMIT deviceVisibilityChanged(id, true);
            Q_EMIT deviceListChanged();
        }
    } else {
        qCDebug(KDECONNECT_CORE) << "It is a new device" << identityPacket.get<QString>(QStringLiteral("deviceName"));
        Device* device = new Device(this, identityPacket, dl);

        //we discard the connections that we created but it's not paired.
        if (!isDiscoveringDevices() && !device->isTrusted() && !dl->linkShouldBeKeptAlive()) {
            device->deleteLater();
        } else {
            addDevice(device);
        }
    }
}

void Daemon::onDeviceStatusChanged()
{
    Device* device = (Device*)sender();

    //qCDebug(KDECONNECT_CORE) << "Device" << device->name() << "status changed. Reachable:" << device->isReachable() << ". Paired: " << device->isPaired();

    if (!device->isReachable() && !device->isTrusted()) {
        //qCDebug(KDECONNECT_CORE) << "Destroying device" << device->name();
        removeDevice(device);
    } else {
        Q_EMIT deviceVisibilityChanged(device->id(), device->isReachable());
        Q_EMIT deviceListChanged();
    }

}

void Daemon::setAnnouncedName(const QString& name)
{
    qCDebug(KDECONNECT_CORE) << "Announcing name";
    KdeConnectConfig::instance().setName(name);
    forceOnNetworkChange();
    Q_EMIT announcedNameChanged(name);
}

void Daemon::setCustomDevices(const QStringList& addresses)
{
    auto& config = KdeConnectConfig::instance();

    auto customDevices = config.customDevices();
    if (customDevices != addresses) {
        qCDebug(KDECONNECT_CORE) << "Changed list of custom device addresses:" << addresses;
        config.setCustomDevices(addresses);
        Q_EMIT customDevicesChanged(addresses);

        forceOnNetworkChange();
    }
}

QStringList Daemon::customDevices() const
{
    return KdeConnectConfig::instance().customDevices();
}

QString Daemon::announcedName()
{
    return KdeConnectConfig::instance().name();
}

QNetworkAccessManager* Daemon::networkAccessManager()
{
    static QPointer<QNetworkAccessManager> manager;
    if (!manager) {
        manager = new QNetworkAccessManager(this);
    }
    return manager;
}

QList<Device*> Daemon::devicesList() const
{
    return d->m_devices.values();
}

bool Daemon::isDiscoveringDevices() const
{
    return !d->m_discoveryModeAcquisitions.isEmpty();
}

QString Daemon::deviceIdByName(const QString& name) const
{
    for (Device* device : qAsConst(d->m_devices)) {
        if (device->name() == name && device->isTrusted())
            return device->id();
    }
    return {};
}

void Daemon::addDevice(Device* device)
{
    const QString id = device->id();
    connect(device, &Device::reachableChanged, this, &Daemon::onDeviceStatusChanged);
    connect(device, &Device::trustedChanged, this, &Daemon::onDeviceStatusChanged);
    connect(device, &Device::hasPairingRequestsChanged, this, &Daemon::pairingRequestsChanged);
    connect(device, &Device::hasPairingRequestsChanged, this, [this, device](bool hasPairingRequests) {
        if (hasPairingRequests)
            askPairingConfirmation(device);
    } );
    d->m_devices[id] = device;

    Q_EMIT deviceAdded(id);
    Q_EMIT deviceListChanged();
}

QStringList Daemon::pairingRequests() const
{
    QStringList ret;
    for(Device* dev: qAsConst(d->m_devices)) {
        if (dev->hasPairingRequests())
            ret += dev->id();
    }
    return ret;
}

Daemon::~Daemon()
{

}

QString Daemon::selfId() const
{
    return KdeConnectConfig::instance().deviceId();
}
