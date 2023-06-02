/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "device.h"

#include <QSet>
#include <QSslCertificate>
#include <QSslKey>
#include <QVector>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

#include "backends/devicelink.h"
#include "backends/lan/landevicelink.h"
#include "backends/linkprovider.h"
#include "core_debug.h"
#include "daemon.h"
#include "dbushelper.h"
#include "kdeconnectconfig.h"
#include "kdeconnectplugin.h"
#include "networkpacket.h"
#include "pluginloader.h"

// In older Qt released, qAsConst isnt available
#include "qtcompat_p.h"

class Device::DevicePrivate
{
public:
    DevicePrivate(const QString &id)
        : m_deviceId(id)
    {
    }

    ~DevicePrivate()
    {
        qDeleteAll(m_deviceLinks);
        m_deviceLinks.clear();
    }

    const QString m_deviceId;
    QString m_deviceName;
    DeviceType m_deviceType;
    int m_protocolVersion;

    QVector<DeviceLink *> m_deviceLinks;
    QHash<QString, KdeConnectPlugin *> m_plugins;

    QMultiMap<QString, KdeConnectPlugin *> m_pluginsByIncomingCapability;
    QSet<QString> m_supportedPlugins;
    QSet<QString> m_allPlugins;
    PairingHandler* m_pairingHandler;
};

static void warn(const QString &info)
{
    qWarning() << "Device pairing error" << info;
}

Device::Device(QObject *parent, const QString &id)
    : QObject(parent)
    , d(new Device::DevicePrivate(id))
{
    d->m_pairingHandler = new PairingHandler(this, PairState::Paired);

    d->m_protocolVersion = NetworkPacket::s_protocolVersion;
    KdeConnectConfig::DeviceInfo info = KdeConnectConfig::instance().getTrustedDevice(d->m_deviceId);

    d->m_deviceName = info.deviceName;
    d->m_deviceType = str2type(info.deviceType);

    // Register in bus
    QDBusConnection::sessionBus().registerObject(dbusPath(), this, QDBusConnection::ExportScriptableContents | QDBusConnection::ExportAdaptors);

    // Assume every plugin is supported until addLink is called and we can get the actual list
    d->m_allPlugins = PluginLoader::instance()->getPluginList().toSet();
    d->m_supportedPlugins = d->m_allPlugins;

    connect(d->m_pairingHandler, &PairingHandler::incomingPairRequest, this, &Device::pairingHandler_incomingPairRequest);
    connect(d->m_pairingHandler, &PairingHandler::pairingFailed, this, &Device::pairingHandler_pairingFailed);
    connect(d->m_pairingHandler, &PairingHandler::pairingSuccessful, this, &Device::pairingHandler_pairingSuccessful);
    connect(d->m_pairingHandler, &PairingHandler::unpaired, this, &Device::pairingHandler_unpaired);
}

Device::Device(QObject *parent, const NetworkPacket &identityPacket, DeviceLink *dl)
    : QObject(parent)
    , d(new Device::DevicePrivate(identityPacket.get<QString>(QStringLiteral("deviceId"))))
{
    d->m_pairingHandler = new PairingHandler(this, PairState::NotPaired);
    d->m_deviceName = identityPacket.get<QString>(QStringLiteral("deviceName"));
    d->m_allPlugins = PluginLoader::instance()->getPluginList().toSet();

    addLink(identityPacket, dl);

    // Register in bus
    QDBusConnection::sessionBus().registerObject(dbusPath(), this, QDBusConnection::ExportScriptableContents | QDBusConnection::ExportAdaptors);

    connect(this, &Device::pairingFailed, this, &warn);

    connect(this, &Device::reachableChanged, this, &Device::statusIconNameChanged);
    connect(this, &Device::pairStateChanged, this, &Device::statusIconNameChanged);

    connect(d->m_pairingHandler, &PairingHandler::incomingPairRequest, this, &Device::pairingHandler_incomingPairRequest);
    connect(d->m_pairingHandler, &PairingHandler::pairingFailed, this, &Device::pairingHandler_pairingFailed);
    connect(d->m_pairingHandler, &PairingHandler::pairingSuccessful, this, &Device::pairingHandler_pairingSuccessful);
    connect(d->m_pairingHandler, &PairingHandler::unpaired, this, &Device::pairingHandler_unpaired);
}

Device::~Device()
{
    delete d;
}

QString Device::id() const
{
    return d->m_deviceId;
}

QString Device::name() const
{
    return d->m_deviceName;
}

QString Device::type() const
{
    return type2str(d->m_deviceType);
}

bool Device::isReachable() const
{
    return !d->m_deviceLinks.isEmpty();
}

int Device::protocolVersion()
{
    return d->m_protocolVersion;
}

QStringList Device::supportedPlugins() const
{
    return d->m_supportedPlugins.toList();
}

bool Device::hasPlugin(const QString &name) const
{
    return d->m_plugins.contains(name);
}

QStringList Device::loadedPlugins() const
{
    return d->m_plugins.keys();
}

void Device::reloadPlugins()
{
    QHash<QString, KdeConnectPlugin *> newPluginMap, oldPluginMap = d->m_plugins;
    QMultiMap<QString, KdeConnectPlugin *> newPluginsByIncomingCapability;

    if (isPaired() && isReachable()) { // Do not load any plugin for unpaired devices, nor useless loading them for unreachable devices

        PluginLoader *loader = PluginLoader::instance();

        for (const QString &pluginName : qAsConst(d->m_supportedPlugins)) {
            const KPluginMetaData service = loader->getPluginInfo(pluginName);

            const bool pluginEnabled = isPluginEnabled(pluginName);
            const QSet<QString> incomingCapabilities =
                KPluginMetaData::readStringList(service.rawData(), QStringLiteral("X-KdeConnect-SupportedPacketType")).toSet();

            if (pluginEnabled) {
                KdeConnectPlugin *plugin = d->m_plugins.take(pluginName);

                if (!plugin) {
                    plugin = loader->instantiatePluginForDevice(pluginName, this);
                }
                Q_ASSERT(plugin);

                for (const QString &interface : incomingCapabilities) {
                    newPluginsByIncomingCapability.insert(interface, plugin);
                }

                newPluginMap[pluginName] = plugin;
            }
        }
    }

    const bool differentPlugins = oldPluginMap != newPluginMap;

    // Erase all left plugins in the original map (meaning that we don't want
    // them anymore, otherwise they would have been moved to the newPluginMap)
    qDeleteAll(d->m_plugins);
    d->m_plugins = newPluginMap;
    d->m_pluginsByIncomingCapability = newPluginsByIncomingCapability;

    QDBusConnection bus = QDBusConnection::sessionBus();
    for (KdeConnectPlugin *plugin : qAsConst(d->m_plugins)) {
        // TODO: see how it works in Android (only done once, when created)
        plugin->connected();

        const QString dbusPath = plugin->dbusPath();
        if (!dbusPath.isEmpty()) {
            bus.registerObject(dbusPath,
                               plugin,
                               QDBusConnection::ExportAllProperties | QDBusConnection::ExportScriptableInvokables | QDBusConnection::ExportScriptableSignals
                                   | QDBusConnection::ExportScriptableSlots);
        }
    }
    if (differentPlugins) {
        Q_EMIT pluginsChanged();
    }
}

QString Device::pluginsConfigFile() const
{
    return KdeConnectConfig::instance().deviceConfigDir(id()).absoluteFilePath(QStringLiteral("config"));
}

void Device::requestPairing()
{
    qCDebug(KDECONNECT_CORE) << "Request pairing";
    d->m_pairingHandler->requestPairing();
    Q_EMIT pairStateChanged(pairState());
}

void Device::unpair()
{
    qCDebug(KDECONNECT_CORE) << "Request unpairing";
    d->m_pairingHandler->unpair();
}

void Device::acceptPairing()
{
    qCDebug(KDECONNECT_CORE) << "Accept pairing";
    d->m_pairingHandler->acceptPairing();
}

void Device::cancelPairing()
{
    qCDebug(KDECONNECT_CORE) << "Cancel pairing";
    d->m_pairingHandler->cancelPairing();
}

void Device::pairingHandler_incomingPairRequest()
{
    Q_ASSERT(d->m_pairingHandler->pairState() == PairState::RequestedByPeer);
    Q_EMIT pairStateChanged(pairState());
}

void Device::pairingHandler_pairingSuccessful()
{
    Q_ASSERT(d->m_pairingHandler->pairState() == PairState::Paired);
    KdeConnectConfig::instance().addTrustedDevice(id(), name(), type());
    KdeConnectConfig::instance().setDeviceProperty(d->m_deviceId, QStringLiteral("certificate"), QString::fromLatin1(certificate().toPem()));
    reloadPlugins(); // Will load/unload plugins
    Q_EMIT pairStateChanged(pairState());
}

void Device::pairingHandler_pairingFailed(const QString &errorMessage)
{
    Q_ASSERT(d->m_pairingHandler->pairState() == PairState::NotPaired);
    Q_EMIT pairingFailed(errorMessage);
    Q_EMIT pairStateChanged(pairState());
}

void Device::pairingHandler_unpaired()
{
    Q_ASSERT(d->m_pairingHandler->pairState() == PairState::NotPaired);
    qCDebug(KDECONNECT_CORE) << "Unpaired";
    KdeConnectConfig::instance().removeTrustedDevice(id());
    reloadPlugins(); // Will load/unload plugins
    Q_EMIT pairStateChanged(pairState());
}

static bool lessThan(DeviceLink *p1, DeviceLink *p2)
{
    return p1->provider()->priority() > p2->provider()->priority();
}

void Device::addLink(const NetworkPacket &identityPacket, DeviceLink *link)
{
    // qCDebug(KDECONNECT_CORE) << "Adding link to" << id() << "via" << link->provider();

    setName(identityPacket.get<QString>(QStringLiteral("deviceName")));
    setType(identityPacket.get<QString>(QStringLiteral("deviceType")));

    if (d->m_deviceLinks.contains(link)) {
        return;
    }

    d->m_protocolVersion = identityPacket.get<int>(QStringLiteral("protocolVersion"), -1);
    if (d->m_protocolVersion != NetworkPacket::s_protocolVersion) {
        qCWarning(KDECONNECT_CORE) << d->m_deviceName << "- warning, device uses a different protocol version" << d->m_protocolVersion << "expected"
                                   << NetworkPacket::s_protocolVersion;
    }

    connect(link, &QObject::destroyed, this, &Device::linkDestroyed);

    d->m_deviceLinks.append(link);

    // Theoretically we will never add two links from the same provider (the provider should destroy
    // the old one before this is called), so we do not have to worry about destroying old links.
    //-- Actually, we should not destroy them or the provider will store an invalid ref!

    connect(link, &DeviceLink::receivedPacket, this, &Device::privateReceivedPacket);

    std::sort(d->m_deviceLinks.begin(), d->m_deviceLinks.end(), lessThan);

    const bool capabilitiesSupported = identityPacket.has(QStringLiteral("incomingCapabilities")) || identityPacket.has(QStringLiteral("outgoingCapabilities"));
    if (capabilitiesSupported) {
        const QSet<QString> outgoingCapabilities = identityPacket.get<QStringList>(QStringLiteral("outgoingCapabilities")).toSet(),
                            incomingCapabilities = identityPacket.get<QStringList>(QStringLiteral("incomingCapabilities")).toSet();

        d->m_supportedPlugins = PluginLoader::instance()->pluginsForCapabilities(incomingCapabilities, outgoingCapabilities);
        // qDebug() << "new plugins for" << m_deviceName << m_supportedPlugins << incomingCapabilities << outgoingCapabilities;
    } else {
        d->m_supportedPlugins = PluginLoader::instance()->getPluginList().toSet();
    }

    reloadPlugins();

    if (d->m_deviceLinks.size() == 1) {
        Q_EMIT reachableChanged(true);
    }
}

void Device::linkDestroyed(QObject *o)
{
    removeLink(static_cast<DeviceLink *>(o));
}

void Device::removeLink(DeviceLink *link)
{
    d->m_deviceLinks.removeAll(link);

    // qCDebug(KDECONNECT_CORE) << "RemoveLink" << m_deviceLinks.size() << "links remaining";

    if (d->m_deviceLinks.isEmpty()) {
        reloadPlugins();
        Q_EMIT reachableChanged(false);
    }
}

bool Device::sendPacket(NetworkPacket &np)
{
    Q_ASSERT(isPaired() || np.type() == PACKET_TYPE_PAIR);

    // Maybe we could block here any packet that is not an identity or a pairing packet to prevent sending non encrypted data
    for (DeviceLink *dl : qAsConst(d->m_deviceLinks)) {
        if (dl->sendPacket(np))
            return true;
    }

    return false;
}

void Device::privateReceivedPacket(const NetworkPacket &np)
{
    if (np.type() == PACKET_TYPE_PAIR) {
        d->m_pairingHandler->packetReceived(np);
    } else if (isPaired()) {
        const QList<KdeConnectPlugin *> plugins = d->m_pluginsByIncomingCapability.values(np.type());
        if (plugins.isEmpty()) {
            qWarning() << "discarding unsupported packet" << np.type() << "for" << name();
        }
        for (KdeConnectPlugin *plugin : plugins) {
            plugin->receivePacket(np);
        }
    } else {
        qCDebug(KDECONNECT_CORE) << "device" << name() << "not paired, ignoring packet" << np.type();
        unpair();
    }
}

PairState Device::pairState() const
{
    return d->m_pairingHandler->pairState();
}

bool Device::isPaired() const
{
    return d->m_pairingHandler->pairState() == PairState::Paired;
}

bool Device::isPairRequested() const
{
    return d->m_pairingHandler->pairState() == PairState::Requested;
}


bool Device::isPairRequestedByPeer() const
{
    return d->m_pairingHandler->pairState() == PairState::RequestedByPeer;
}


QStringList Device::availableLinks() const
{
    QStringList sl;
    sl.reserve(d->m_deviceLinks.size());
    for (DeviceLink *dl : qAsConst(d->m_deviceLinks)) {
        sl.append(dl->provider()->name());
    }
    return sl;
}

QHostAddress Device::getLocalIpAddress() const
{
    for (DeviceLink *dl : qAsConst(d->m_deviceLinks)) {
        LanDeviceLink *ldl = dynamic_cast<LanDeviceLink *>(dl);
        if (ldl) {
            return ldl->hostAddress();
        }
    }
    return QHostAddress::Null;
}

Device::DeviceType Device::str2type(const QString &deviceType)
{
    if (deviceType == QLatin1String("desktop"))
        return Desktop;
    if (deviceType == QLatin1String("laptop"))
        return Laptop;
    if (deviceType == QLatin1String("smartphone") || deviceType == QLatin1String("phone"))
        return Phone;
    if (deviceType == QLatin1String("tablet"))
        return Tablet;
    if (deviceType == QLatin1String("tv"))
        return Tv;
    return Unknown;
}

QString Device::type2str(Device::DeviceType deviceType)
{
    if (deviceType == Desktop)
        return QStringLiteral("desktop");
    if (deviceType == Laptop)
        return QStringLiteral("laptop");
    if (deviceType == Phone)
        return QStringLiteral("smartphone");
    if (deviceType == Tablet)
        return QStringLiteral("tablet");
    if (deviceType == Tv)
        return QStringLiteral("tv");
    return QStringLiteral("unknown");
}

QString Device::statusIconName() const
{
    return iconForStatus(isReachable(), isPaired());
}

QString Device::iconName() const
{
    return iconForStatus(true, false);
}

QString Device::iconForStatus(bool reachable, bool trusted) const
{
    Device::DeviceType deviceType = d->m_deviceType;
    if (deviceType == Device::Unknown) {
        deviceType = Device::Phone; // Assume phone if we don't know the type
    } else if (deviceType == Device::Desktop) {
        deviceType = Device::Device::Laptop; // We don't have desktop icon yet
    }

    QString status = (reachable ? (trusted ? QStringLiteral("connected") : QStringLiteral("disconnected")) : QStringLiteral("trusted"));
    QString type = type2str(deviceType);

    return type + status;
}

void Device::setName(const QString &name)
{
    if (d->m_deviceName != name) {
        d->m_deviceName = name;
        KdeConnectConfig::instance().setDeviceProperty(d->m_deviceId, QStringLiteral("name"), name);
        Q_EMIT nameChanged(name);
    }
}

void Device::setType(const QString &strtype)
{
    auto type = str2type(strtype);
    if (d->m_deviceType != type) {
        d->m_deviceType = type;
        KdeConnectConfig::instance().setDeviceProperty(d->m_deviceId, QStringLiteral("type"), strtype);
        Q_EMIT typeChanged(strtype);
    }
}

KdeConnectPlugin *Device::plugin(const QString &pluginName) const
{
    return d->m_plugins[pluginName];
}

void Device::setPluginEnabled(const QString &pluginName, bool enabled)
{
    if (!d->m_allPlugins.contains(pluginName)) {
        return;
    }

    KConfigGroup pluginStates = KSharedConfig::openConfig(pluginsConfigFile())->group("Plugins");

    const QString enabledKey = pluginName + QStringLiteral("Enabled");
    pluginStates.writeEntry(enabledKey, enabled);
    reloadPlugins();
}

bool Device::isPluginEnabled(const QString &pluginName) const
{
    const QString enabledKey = pluginName + QStringLiteral("Enabled");
    KConfigGroup pluginStates = KSharedConfig::openConfig(pluginsConfigFile())->group("Plugins");

    return (pluginStates.hasKey(enabledKey) ? pluginStates.readEntry(enabledKey, false)
                                            : PluginLoader::instance()->getPluginInfo(pluginName).isEnabledByDefault());
}

QString Device::encryptionInfo() const
{
    QString result;
    const QCryptographicHash::Algorithm digestAlgorithm = QCryptographicHash::Algorithm::Sha256;

    QString localChecksum = QString::fromLatin1(KdeConnectConfig::instance().certificate().digest(digestAlgorithm).toHex());
    for (int i = 2; i < localChecksum.size(); i += 3) {
        localChecksum.insert(i, QStringLiteral(":")); // Improve readability
    }
    result += i18n("SHA256 fingerprint of your device certificate is: %1\n", localChecksum);

    std::string remotePem = KdeConnectConfig::instance().getDeviceProperty(id(), QStringLiteral("certificate")).toStdString();
    QSslCertificate remoteCertificate = QSslCertificate(QByteArray(remotePem.c_str(), (int)remotePem.size()));
    QString remoteChecksum = QString::fromLatin1(remoteCertificate.digest(digestAlgorithm).toHex());
    for (int i = 2; i < remoteChecksum.size(); i += 3) {
        remoteChecksum.insert(i, QStringLiteral(":")); // Improve readability
    }
    result += i18n("SHA256 fingerprint of remote device certificate is: %1\n", remoteChecksum);

    return result;
}

QSslCertificate Device::certificate() const
{
    if (!d->m_deviceLinks.isEmpty()) {
        return d->m_deviceLinks[0]->certificate();
    }
    return QSslCertificate();
}

QByteArray Device::verificationKey() const
{
    auto a = KdeConnectConfig::instance().certificate().publicKey().toDer();
    auto b = certificate().publicKey().toDer();
    if (a < b) {
        std::swap(a, b);
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(a);
    hash.addData(b);
    return hash.result().toHex();
}

QString Device::pluginIconName(const QString &pluginName)
{
    if (hasPlugin(pluginName)) {
        return d->m_plugins[pluginName]->iconName();
    }
    return QString();
}
