/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "daemon.h"

#include <QDBusMetaType>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QProcess>

#include "core_debug.h"
#include "dbushelper.h"
#include "kdeconnectconfig.h"
#include "networkpacket.h"
#include "notificationserverinfo.h"

#ifdef KDECONNECT_BLUETOOTH
#include "backends/bluetooth/asynclinkprovider.h"
#include "backends/bluetooth/bluetoothlinkprovider.h"
#endif

#include "backends/devicelink.h"
#include "backends/lan/lanlinkprovider.h"
#include "backends/linkprovider.h"
#include "backends/loopback/loopbacklinkprovider.h"
#include "device.h"

static Daemon *s_instance = nullptr;

struct DaemonPrivate {
    // Different ways to find devices and connect to them
    QSet<LinkProvider *> m_linkProviders;

    // Every known device
    QMap<QString, Device *> m_devices;

    bool m_testMode;
};

Daemon *Daemon::instance()
{
    Q_ASSERT(s_instance != nullptr);
    return s_instance;
}

Daemon::Daemon(QObject *parent, bool testMode)
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

    // Register on DBus
    // This must happen as early as possible in the process startup to ensure
    // the absolute minimum amount of blocking on logon/autostart

    qDBusRegisterMetaType<QMap<QString, QString>>();
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.kdeconnect"));
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/modules/kdeconnect"), this, QDBusConnection::ExportScriptableContents);

    qCDebug(KDECONNECT_CORE) << "DBus registration complete";

    auto configInstance = KdeConnectConfig::instance();
    const auto disabledLinkProviders = configInstance.disabledLinkProviders();

    qCDebug(KDECONNECT_CORE) << "Constructing Backends";
    // Load backends
    if (d->m_testMode) {
        d->m_linkProviders.insert(new LoopbackLinkProvider());
        qCDebug(KDECONNECT_CORE) << "Constructed LoopbackLink Backend";
    } else {
        d->m_linkProviders.insert(new LanLinkProvider(false, disabledLinkProviders.contains(QStringLiteral("LanLinkProvider"))));
        qCDebug(KDECONNECT_CORE) << "Constructed LanLinkProvider Backend";
#ifdef KDECONNECT_BLUETOOTH
        d->m_linkProviders.insert(new AsyncLinkProvider(disabledLinkProviders.contains(QStringLiteral("AsyncLinkProvider"))));
        qCDebug(KDECONNECT_CORE) << "Constructed AsyncLinkProvider<AsyncLinkProvider> Backend";
#endif
#ifdef KDECONNECT_LOOPBACK
        d->m_linkProviders.insert(new LoopbackLinkProvider());
        qCDebug(KDECONNECT_CORE) << "Constructed LoopbackLinkProvider Backend";
#endif
    }
    Q_EMIT linkProvidersChanged(linkProviders());

    qCDebug(KDECONNECT_CORE) << "Backends loaded";

    // Read remembered paired devices
    const QStringList &list = KdeConnectConfig::instance().trustedDevices();
    for (const QString &id : list) {
        Device *d = new Device(this, id);
        // prune away devices with malformed certificates
        if (d->hasInvalidCertificate()) {
            qCDebug(KDECONNECT_CORE) << "Certificate for device " << id << "invalid, deleting the device";
            KdeConnectConfig::instance().removeTrustedDevice(id);
        } else {
            addDevice(d);
        }
    }

    qCDebug(KDECONNECT_CORE) << "Paired devices added";

    // Listen to new devices
    for (LinkProvider *a : std::as_const(d->m_linkProviders)) {
        connect(a, &LinkProvider::onConnectionReceived, this, &Daemon::onNewDeviceLink);
        a->onStart();
    }

    qCDebug(KDECONNECT_CORE) << "Link providers started";

    NotificationServerInfo::instance().init();

    qCDebug(KDECONNECT_CORE) << "Daemon start completed";
}

void Daemon::removeDevice(Device *device)
{
    d->m_devices.remove(device->id());
    device->deleteLater();
    Q_EMIT deviceRemoved(device->id());
    Q_EMIT deviceListChanged();
}

void Daemon::forceOnNetworkChange()
{
    qCDebug(KDECONNECT_CORE) << "Sending onNetworkChange to" << d->m_linkProviders.size() << "LinkProviders";
    for (LinkProvider *a : std::as_const(d->m_linkProviders)) {
        qCDebug(KDECONNECT_CORE) << "Sending onNetworkChange to:" << a->name();
        a->onNetworkChange();
    }
}

Device *Daemon::getDevice(const QString &deviceId)
{
    for (Device *device : std::as_const(d->m_devices)) {
        if (device->id() == deviceId) {
            return device;
        }
    }
    return nullptr;
}

QSet<LinkProvider *> Daemon::getLinkProviders() const
{
    return d->m_linkProviders;
}

QStringList Daemon::linkProviders() const
{
    auto configInstance = KdeConnectConfig::instance();
    const auto disabledLinkProviders = configInstance.disabledLinkProviders();
    QStringList returnValue;

    for (LinkProvider *a : std::as_const(d->m_linkProviders)) {
        QString line(a->displayName() + u'|' + a->name());

        if (disabledLinkProviders.contains(a->name())) {
            line += QStringLiteral("|disabled");
        } else {
            line += QStringLiteral("|enabled");
        }

        returnValue.append(line);
    }
    returnValue.sort();
    return returnValue;
}

void Daemon::setLinkProviderState(const QString &linkProviderName, bool enabled)
{
    qCDebug(KDECONNECT_CORE) << "setLinkProviderState called" << linkProviderName << enabled;

    KdeConnectConfig configInstance = KdeConnectConfig::instance();
    LinkProvider *providerByName = nullptr;
    const auto allLinkProviders = getLinkProviders();
    for (LinkProvider *provider : allLinkProviders) {
        if (provider->name() == linkProviderName) {
            providerByName = provider;
            break;
        }
    }
    Q_ASSERT(providerByName);

    auto disabledLinkProviders = configInstance.disabledLinkProviders();
    if (!enabled && !disabledLinkProviders.contains(linkProviderName)) {
        qCDebug(KDECONNECT_CORE) << "disabling" << linkProviderName;
        disabledLinkProviders.append(linkProviderName);
        providerByName->disable();
    } else if (enabled && disabledLinkProviders.contains(linkProviderName)) {
        qCDebug(KDECONNECT_CORE) << "enabling " << linkProviderName;
        disabledLinkProviders.removeAll(linkProviderName);
        providerByName->enable();
    } else {
        qCDebug(KDECONNECT_CORE) << "No changes to the disabledLinkProviders have been made, returning";
        return;
    }

    configInstance.setDisabledLinkProviders(disabledLinkProviders);
    Q_EMIT linkProvidersChanged(linkProviders());
}

QStringList Daemon::devices(bool onlyReachable, bool onlyTrusted) const
{
    QStringList ret;
    for (Device *device : std::as_const(d->m_devices)) {
        if (onlyReachable && !device->isReachable())
            continue;
        if (onlyTrusted && !device->isPaired())
            continue;
        ret.append(device->id());
    }
    return ret;
}

QMap<QString, QString> Daemon::deviceNames(bool onlyReachable, bool onlyTrusted) const
{
    QMap<QString, QString> ret;
    for (Device *device : std::as_const(d->m_devices)) {
        if (onlyReachable && !device->isReachable())
            continue;
        if (onlyTrusted && !device->isPaired())
            continue;
        ret[device->id()] = device->name();
    }
    return ret;
}

void Daemon::onNewDeviceLink(DeviceLink *link)
{
    QString id = link->deviceId();

    qCDebug(KDECONNECT_CORE) << "Device discovered" << id << "via link with priority" << link->priority();

    if (d->m_devices.contains(id)) {
        qCDebug(KDECONNECT_CORE) << "It is a known device" << link->deviceInfo().name;
        Device *device = d->m_devices[id];
        bool wasReachable = device->isReachable();
        device->addLink(link);
        if (!wasReachable) {
            Q_EMIT deviceVisibilityChanged(id, true);
            Q_EMIT deviceListChanged();
        }
    } else {
        qCDebug(KDECONNECT_CORE) << "It is a new device" << link->deviceInfo().name;
        Device *device = new Device(this, link);
        addDevice(device);
    }
}

void Daemon::onDeviceStatusChanged()
{
    Device *device = (Device *)sender();

    qCDebug(KDECONNECT_CORE) << "Device" << device->name() << "status changed. Reachable:" << device->isReachable() << ". Paired: " << device->isPaired();

    if (!device->isReachable() && !device->isPaired()) {
        qCDebug(KDECONNECT_CORE) << "Destroying device" << device->name();
        removeDevice(device);
    } else {
        Q_EMIT deviceVisibilityChanged(device->id(), device->isReachable());
        Q_EMIT deviceListChanged();
    }
}

void Daemon::setAnnouncedName(const QString &name)
{
    QString filteredName = DeviceInfo::filterName(name);
    qCDebug(KDECONNECT_CORE) << "Announcing name";
    KdeConnectConfig::instance().setName(filteredName);
    forceOnNetworkChange();
    Q_EMIT announcedNameChanged(filteredName);
}

void Daemon::setCustomDevices(const QStringList &addresses)
{
    auto &config = KdeConnectConfig::instance();

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

QNetworkAccessManager *Daemon::networkAccessManager()
{
    static QPointer<QNetworkAccessManager> manager;
    if (!manager) {
        manager = new QNetworkAccessManager(this);
    }
    return manager;
}

QList<Device *> Daemon::devicesList() const
{
    return d->m_devices.values();
}

QString Daemon::deviceIdByName(const QString &name) const
{
    for (Device *device : std::as_const(d->m_devices)) {
        if (device->name() == name) {
            return device->id();
        }
    }
    return {};
}

void Daemon::addDevice(Device *device)
{
    const QString id = device->id();
    connect(device, &Device::reachableChanged, this, &Daemon::onDeviceStatusChanged);
    connect(device, &Device::pairStateChanged, this, &Daemon::onDeviceStatusChanged);
    connect(device, &Device::pairStateChanged, this, &Daemon::pairingRequestsChanged);
    connect(device, &Device::pairStateChanged, this, [this, device](int pairStateAsInt) {
        PairState pairState = (PairState)pairStateAsInt;
        if (pairState == PairState::RequestedByPeer)
            askPairingConfirmation(device);
    });
    d->m_devices[id] = device;

    Q_EMIT deviceAdded(id);
    Q_EMIT deviceListChanged();
}

QStringList Daemon::pairingRequests() const
{
    QStringList ret;
    for (Device *dev : std::as_const(d->m_devices)) {
        if (dev->isPairRequestedByPeer())
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

#include "moc_daemon.cpp"
