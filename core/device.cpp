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

class Device::DevicePrivate
{
public:
    DevicePrivate(const DeviceInfo &deviceInfo)
        : m_deviceInfo(deviceInfo)
    {
    }

    ~DevicePrivate()
    {
        qDeleteAll(m_deviceLinks);
        m_deviceLinks.clear();
    }

    DeviceInfo m_deviceInfo;

    QVector<DeviceLink *> m_deviceLinks;
    QHash<QString, KdeConnectPlugin *> m_plugins;

    QMultiMap<QString, KdeConnectPlugin *> m_pluginsByIncomingCapability;
    QSet<QString> m_supportedPlugins;
    PairingHandler *m_pairingHandler;
};

static void warn(const QString &info)
{
    qWarning() << "Device pairing error" << info;
}

Device::Device(QObject *parent, const QString &id)
    : QObject(parent)
{
    DeviceInfo info = KdeConnectConfig::instance().getTrustedDevice(id);
    d = new Device::DevicePrivate(info);

    d->m_pairingHandler = new PairingHandler(this, PairState::Paired);
    const auto supported = PluginLoader::instance()->getPluginList();
    d->m_supportedPlugins = QSet(supported.begin(), supported.end()); // Assume every plugin is supported until we get the capabilities

    // Register in bus
    QDBusConnection::sessionBus().registerObject(dbusPath(), this, QDBusConnection::ExportScriptableContents | QDBusConnection::ExportAdaptors);

    connect(d->m_pairingHandler, &PairingHandler::incomingPairRequest, this, &Device::pairingHandler_incomingPairRequest);
    connect(d->m_pairingHandler, &PairingHandler::pairingFailed, this, &Device::pairingHandler_pairingFailed);
    connect(d->m_pairingHandler, &PairingHandler::pairingSuccessful, this, &Device::pairingHandler_pairingSuccessful);
    connect(d->m_pairingHandler, &PairingHandler::unpaired, this, &Device::pairingHandler_unpaired);
}

Device::Device(QObject *parent, DeviceLink *dl)
    : QObject(parent)
{
    d = new Device::DevicePrivate(dl->deviceInfo());

    d->m_pairingHandler = new PairingHandler(this, PairState::NotPaired);
    const auto supported = PluginLoader::instance()->getPluginList();
    d->m_supportedPlugins = QSet(supported.begin(), supported.end()); // Assume every plugin is supported until we get the capabilities

    addLink(dl);

    // Register in bus
    QDBusConnection::sessionBus().registerObject(dbusPath(), this, QDBusConnection::ExportScriptableContents | QDBusConnection::ExportAdaptors);

    connect(this, &Device::pairingFailed, this, &warn);

    connect(this, &Device::reachableChanged, this, &Device::statusIconNameChanged);
    connect(this, &Device::pairStateChanged, this, &Device::statusIconNameChanged);
    connect(this, &Device::typeChanged, this, &Device::statusIconNameChanged);

    connect(d->m_pairingHandler, &PairingHandler::incomingPairRequest, this, &Device::pairingHandler_incomingPairRequest);
    connect(d->m_pairingHandler, &PairingHandler::pairingFailed, this, &Device::pairingHandler_pairingFailed);
    connect(d->m_pairingHandler, &PairingHandler::pairingSuccessful, this, &Device::pairingHandler_pairingSuccessful);
    connect(d->m_pairingHandler, &PairingHandler::unpaired, this, &Device::pairingHandler_unpaired);

    if (protocolVersion() != NetworkPacket::s_protocolVersion) {
        qCWarning(KDECONNECT_CORE) << name() << " uses a different protocol version" << protocolVersion() << "expected" << NetworkPacket::s_protocolVersion;
    }
}

Device::~Device()
{
    delete d;
}

QString Device::id() const
{
    return d->m_deviceInfo.id;
}

QString Device::name() const
{
    return d->m_deviceInfo.name;
}

DeviceType Device::type() const
{
    return d->m_deviceInfo.type;
}

bool Device::isReachable() const
{
    return !d->m_deviceLinks.isEmpty();
}

QStringList Device::reachableAddresses() const
{
    QList<QString> addresses;
    for (const DeviceLink *deviceLink : std::as_const(d->m_deviceLinks)) {
        addresses.append(deviceLink->address());
    }
    return addresses;
}

QStringList Device::activeProviderNames() const
{
    QList<QString> providers;
    for (const DeviceLink *deviceLink : std::as_const(d->m_deviceLinks)) {
        providers.append(deviceLink->providerName());
    }
    return providers;
}

int Device::protocolVersion()
{
    return d->m_deviceInfo.protocolVersion;
}

QStringList Device::supportedPlugins() const
{
    return QList(d->m_supportedPlugins.cbegin(), d->m_supportedPlugins.cend());
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
    qCDebug(KDECONNECT_CORE) << name() << "- reload plugins";

    QHash<QString, KdeConnectPlugin *> newPluginMap, oldPluginMap = d->m_plugins;
    QMultiMap<QString, KdeConnectPlugin *> newPluginsByIncomingCapability;

    if (isPaired() && isReachable()) { // Do not load any plugin for unpaired devices, nor useless loading them for unreachable devices

        PluginLoader *loader = PluginLoader::instance();

        for (const QString &pluginName : std::as_const(d->m_supportedPlugins)) {
            const KPluginMetaData service = loader->getPluginInfo(pluginName);

            const bool pluginEnabled = isPluginEnabled(pluginName);
            const QStringList incomingCapabilities = service.rawData().value(QStringLiteral("X-KdeConnect-SupportedPacketType")).toVariant().toStringList();

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

                plugin->connected();
            }
        }
    }

    const bool differentPlugins = oldPluginMap != newPluginMap;

    // Erase all left plugins in the original map (meaning that we don't want
    // them anymore, otherwise they would have been moved to the newPluginMap)
    qDeleteAll(d->m_plugins);
    d->m_plugins = newPluginMap;
    d->m_pluginsByIncomingCapability = newPluginsByIncomingCapability;

    // Recreate dbus paths for all plugins (new and existing)
    QDBusConnection bus = QDBusConnection::sessionBus();
    for (KdeConnectPlugin *plugin : std::as_const(d->m_plugins)) {
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
    Q_EMIT pairStateChanged(pairStateAsInt());
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
    Q_EMIT pairStateChanged(pairStateAsInt());
}

void Device::pairingHandler_pairingSuccessful()
{
    Q_ASSERT(d->m_pairingHandler->pairState() == PairState::Paired);
    KdeConnectConfig::instance().addTrustedDevice(d->m_deviceInfo);
    reloadPlugins(); // Will load/unload plugins
    Q_EMIT pairStateChanged(pairStateAsInt());
}

void Device::pairingHandler_pairingFailed(const QString &errorMessage)
{
    Q_ASSERT(d->m_pairingHandler->pairState() == PairState::NotPaired);
    Q_EMIT pairingFailed(errorMessage);
    Q_EMIT pairStateChanged(pairStateAsInt());
}

void Device::pairingHandler_unpaired()
{
    Q_ASSERT(d->m_pairingHandler->pairState() == PairState::NotPaired);
    qCDebug(KDECONNECT_CORE) << "Unpaired";
    KdeConnectConfig::instance().removeTrustedDevice(id());
    reloadPlugins(); // Will load/unload plugins
    Q_EMIT pairStateChanged(pairStateAsInt());
}

void Device::addLink(DeviceLink *link)
{
    if (d->m_deviceLinks.contains(link)) {
        return;
    }

    d->m_deviceLinks.append(link);

    std::sort(d->m_deviceLinks.begin(), d->m_deviceLinks.end(), [](DeviceLink *a, DeviceLink *b) {
        return a->priority() > b->priority();
    });

    connect(link, &QObject::destroyed, this, &Device::linkDestroyed);
    connect(link, &DeviceLink::receivedPacket, this, &Device::privateReceivedPacket);

    bool hasChanges = updateDeviceInfo(link->deviceInfo());

    if (d->m_deviceLinks.size() == 1) {
        Q_EMIT reachableChanged(true);
        hasChanges = true;
    }

    if (hasChanges) {
        reloadPlugins();
    }

    Q_EMIT linksChanged();
}

bool Device::updateDeviceInfo(const DeviceInfo &newDeviceInfo)
{
    bool hasChanges = false;
    if (d->m_deviceInfo.name != newDeviceInfo.name || d->m_deviceInfo.type != newDeviceInfo.type
        || d->m_deviceInfo.protocolVersion != newDeviceInfo.protocolVersion) {
        hasChanges = true;
        d->m_deviceInfo.name = newDeviceInfo.name;
        d->m_deviceInfo.type = newDeviceInfo.type;
        d->m_deviceInfo.protocolVersion = newDeviceInfo.protocolVersion;
        Q_EMIT typeChanged(d->m_deviceInfo.type.toString());
        Q_EMIT nameChanged(d->m_deviceInfo.name);
        if (isPaired()) {
            KdeConnectConfig::instance().updateTrustedDeviceInfo(d->m_deviceInfo);
        }
    }

    if (d->m_deviceInfo.outgoingCapabilities != newDeviceInfo.outgoingCapabilities
        || d->m_deviceInfo.incomingCapabilities != newDeviceInfo.incomingCapabilities) {
        if (!newDeviceInfo.incomingCapabilities.isEmpty() && !newDeviceInfo.outgoingCapabilities.isEmpty()) {
            hasChanges = true;
            d->m_deviceInfo.outgoingCapabilities = newDeviceInfo.outgoingCapabilities;
            d->m_deviceInfo.incomingCapabilities = newDeviceInfo.incomingCapabilities;
            d->m_supportedPlugins = PluginLoader::instance()->pluginsForCapabilities(newDeviceInfo.incomingCapabilities, newDeviceInfo.outgoingCapabilities);
            qDebug() << "new capabilities for " << name();
        }
    }

    return hasChanges;
}
bool Device::hasInvalidCertificate()
{
    QDateTime now = QDateTime::currentDateTime();
    return certificate().isNull() || certificate().effectiveDate() >= now || certificate().expiryDate() <= now;
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

    Q_EMIT linksChanged();
}

bool Device::sendPacket(NetworkPacket &np)
{
    Q_ASSERT(isPaired() || np.type() == PACKET_TYPE_PAIR);

    if (!(np.isProtocolPacket() || d->m_deviceInfo.incomingCapabilities.contains(np.type()))) {
        qCDebug(KDECONNECT_CORE) << metaObject()->className() << "Tried to send an unsupported packet type" << np.type() << "to:" << d->m_deviceInfo.name;
        return false;
    }

    // Maybe we could block here any packet that is not an identity or a pairing packet to prevent sending non encrypted data
    for (DeviceLink *dl : std::as_const(d->m_deviceLinks)) {
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

            // If there is a payload close it to not leak sockets
            if (np.payload()) {
                np.payload()->close();
            }
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

int Device::pairStateAsInt() const
{
    return (int)pairState();
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

QHostAddress Device::getLocalIpAddress() const
{
    for (DeviceLink *dl : std::as_const(d->m_deviceLinks)) {
        LanDeviceLink *ldl = dynamic_cast<LanDeviceLink *>(dl);
        if (ldl) {
            return ldl->hostAddress();
        }
    }
    return QHostAddress::Null;
}

QString Device::iconName() const
{
    return d->m_deviceInfo.type.icon();
}

QString Device::statusIconName() const
{
    return d->m_deviceInfo.type.iconForStatus(isReachable(), isPaired());
}

KdeConnectPlugin *Device::plugin(const QString &pluginName) const
{
    return d->m_plugins[pluginName];
}

void Device::setPluginEnabled(const QString &pluginName, bool enabled)
{
    if (!PluginLoader::instance()->doesPluginExist(pluginName)) {
        qWarning() << "Tried to enable a plugin that doesn't exist" << pluginName;
        return;
    }

    KConfigGroup pluginStates = KSharedConfig::openConfig(pluginsConfigFile())->group(QStringLiteral("Plugins"));

    const QString enabledKey = pluginName + QStringLiteral("Enabled");
    pluginStates.writeEntry(enabledKey, enabled);
    reloadPlugins();
}

bool Device::isPluginEnabled(const QString &pluginName) const
{
    const QString enabledKey = pluginName + QStringLiteral("Enabled");
    KConfigGroup pluginStates = KSharedConfig::openConfig(pluginsConfigFile())->group(QStringLiteral("Plugins"));

    return (pluginStates.hasKey(enabledKey) ? pluginStates.readEntry(enabledKey, false)
                                            : PluginLoader::instance()->getPluginInfo(pluginName).isEnabledByDefault());
}

QString Device::encryptionInfo() const
{
    QString result;
    const QCryptographicHash::Algorithm digestAlgorithm = QCryptographicHash::Algorithm::Sha256;

    QString localChecksum = QString::fromLatin1(KdeConnectConfig::instance().certificate().digest(digestAlgorithm).toHex());
    for (int i = 2; i < localChecksum.size(); i += 3) {
        localChecksum.insert(i, QLatin1Char(':')); // Improve readability
    }
    result += i18n("SHA256 fingerprint of your device certificate is: %1\n", localChecksum);

    QString remoteChecksum = QString::fromLatin1(certificate().digest(digestAlgorithm).toHex());
    for (int i = 2; i < remoteChecksum.size(); i += 3) {
        remoteChecksum.insert(i, QLatin1Char(':')); // Improve readability
    }
    result += i18n("SHA256 fingerprint of remote device certificate is: %1\n", remoteChecksum);

    result += i18n("Protocol version: %1\n", d->m_deviceInfo.protocolVersion);

    return result;
}

QSslCertificate Device::certificate() const
{
    return d->m_deviceInfo.certificate;
}

QString Device::verificationKey() const
{
    return d->m_pairingHandler->verificationKey();
}

QString Device::pluginIconName(const QString &pluginName)
{
    if (hasPlugin(pluginName)) {
        return d->m_plugins[pluginName]->iconName();
    }
    return QString();
}

#include "moc_device.cpp"
