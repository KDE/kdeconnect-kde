/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "device.h"

#ifdef interface // MSVC language extension, QDBusConnection uses this as a variable name
#undef interface
#endif

#include <QDBusConnection>
#include <QSslCertificate>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>

#include "core_debug.h"
#include "kdeconnectplugin.h"
#include "pluginloader.h"
#include "backends/devicelink.h"
#include "backends/lan/landevicelink.h"
#include "backends/linkprovider.h"
#include "networkpackage.h"
#include "kdeconnectconfig.h"
#include "daemon.h"

static void warn(const QString& info)
{
    qWarning() << "Device pairing error" << info;
}

Device::Device(QObject* parent, const QString& id)
    : QObject(parent)
    , m_deviceId(id)
    , m_protocolVersion(NetworkPackage::s_protocolVersion) //We don't know it yet
{
    KdeConnectConfig::DeviceInfo info = KdeConnectConfig::instance()->getTrustedDevice(id);

    m_deviceName = info.deviceName;
    m_deviceType = str2type(info.deviceType);

    //Register in bus
    QDBusConnection::sessionBus().registerObject(dbusPath(), this, QDBusConnection::ExportScriptableContents | QDBusConnection::ExportAdaptors);

    //Assume every plugin is supported until addLink is called and we can get the actual list
    m_supportedPlugins = PluginLoader::instance()->getPluginList().toSet();

    connect(this, &Device::pairingError, this, &warn);
}

Device::Device(QObject* parent, const NetworkPackage& identityPackage, DeviceLink* dl)
    : QObject(parent)
    , m_deviceId(identityPackage.get<QString>(QStringLiteral("deviceId")))
    , m_deviceName(identityPackage.get<QString>(QStringLiteral("deviceName")))
{
    addLink(identityPackage, dl);

    //Register in bus
    QDBusConnection::sessionBus().registerObject(dbusPath(), this, QDBusConnection::ExportScriptableContents | QDBusConnection::ExportAdaptors);

    connect(this, &Device::pairingError, this, &warn);
}

Device::~Device()
{
    qDeleteAll(m_deviceLinks);
    m_deviceLinks.clear();
}

bool Device::hasPlugin(const QString& name) const
{
    return m_plugins.contains(name);
}

QStringList Device::loadedPlugins() const
{
    return m_plugins.keys();
}

void Device::reloadPlugins()
{
    QHash<QString, KdeConnectPlugin*> newPluginMap, oldPluginMap = m_plugins;
    QMultiMap<QString, KdeConnectPlugin*> newPluginsByIncomingCapability;

    if (isTrusted() && isReachable()) { //Do not load any plugin for unpaired devices, nor useless loading them for unreachable devices

        PluginLoader* loader = PluginLoader::instance();

        for (const QString& pluginName : qAsConst(m_supportedPlugins)) {
            const KPluginMetaData service = loader->getPluginInfo(pluginName);

            const bool pluginEnabled = isPluginEnabled(pluginName);
            const QSet<QString> incomingCapabilities = KPluginMetaData::readStringList(service.rawData(), QStringLiteral("X-KdeConnect-SupportedPackageType")).toSet();

            if (pluginEnabled) {
                KdeConnectPlugin* plugin = m_plugins.take(pluginName);

                if (!plugin) {
                    plugin = loader->instantiatePluginForDevice(pluginName, this);
                }
                Q_ASSERT(plugin);

                for (const QString& interface : incomingCapabilities) {
                    newPluginsByIncomingCapability.insert(interface, plugin);
                }

                newPluginMap[pluginName] = plugin;
            }
        }
    }

    const bool differentPlugins = oldPluginMap != newPluginMap;

    //Erase all left plugins in the original map (meaning that we don't want
    //them anymore, otherwise they would have been moved to the newPluginMap)
    qDeleteAll(m_plugins);
    m_plugins = newPluginMap;
    m_pluginsByIncomingCapability = newPluginsByIncomingCapability;

    QDBusConnection bus = QDBusConnection::sessionBus();
    for (KdeConnectPlugin* plugin : qAsConst(m_plugins)) {
        //TODO: see how it works in Android (only done once, when created)
        plugin->connected();

        const QString dbusPath = plugin->dbusPath();
        if (!dbusPath.isEmpty()) {
            bus.registerObject(dbusPath, plugin, QDBusConnection::ExportAllProperties | QDBusConnection::ExportScriptableInvokables | QDBusConnection::ExportScriptableSignals | QDBusConnection::ExportScriptableSlots);
        }
    }
    if (differentPlugins) {
        Q_EMIT pluginsChanged();
    }
}

QString Device::pluginsConfigFile() const
{
    return KdeConnectConfig::instance()->deviceConfigDir(id()).absoluteFilePath(QStringLiteral("config"));
}

void Device::requestPair()
{
    if (isTrusted()) {
        Q_EMIT pairingError(i18n("Already paired"));
        return;
    }

    if (!isReachable()) {
        Q_EMIT pairingError(i18n("Device not reachable"));
        return;
    }

    for (DeviceLink* dl : qAsConst(m_deviceLinks)) {
        dl->userRequestsPair();
    }
}

void Device::unpair()
{
    for (DeviceLink* dl : qAsConst(m_deviceLinks)) {
        dl->userRequestsUnpair();
    }
    KdeConnectConfig::instance()->removeTrustedDevice(id());
    Q_EMIT trustedChanged(false);
}

void Device::pairStatusChanged(DeviceLink::PairStatus status)
{
    if (status == DeviceLink::NotPaired) {
        KdeConnectConfig::instance()->removeTrustedDevice(id());

        for (DeviceLink* dl : qAsConst(m_deviceLinks)) {
            if (dl != sender()) {
                dl->setPairStatus(DeviceLink::NotPaired);
            }
        }
    } else {
        KdeConnectConfig::instance()->addTrustedDevice(id(), name(), type());
    }

    reloadPlugins(); //Will load/unload plugins

    bool isTrusted = (status == DeviceLink::Paired);
    Q_EMIT trustedChanged(isTrusted);
    Q_ASSERT(isTrusted == this->isTrusted());
}

static bool lessThan(DeviceLink* p1, DeviceLink* p2)
{
    return p1->provider()->priority() > p2->provider()->priority();
}

void Device::addLink(const NetworkPackage& identityPackage, DeviceLink* link)
{
    //qCDebug(KDECONNECT_CORE) << "Adding link to" << id() << "via" << link->provider();

    setName(identityPackage.get<QString>(QStringLiteral("deviceName")));
    m_deviceType = str2type(identityPackage.get<QString>(QStringLiteral("deviceType")));

    if (m_deviceLinks.contains(link))
        return;

    m_protocolVersion = identityPackage.get<int>(QStringLiteral("protocolVersion"), -1);
    if (m_protocolVersion != NetworkPackage::s_protocolVersion) {
        qCWarning(KDECONNECT_CORE) << m_deviceName << "- warning, device uses a different protocol version" << m_protocolVersion << "expected" << NetworkPackage::s_protocolVersion;
    }

    connect(link, &QObject::destroyed,
            this, &Device::linkDestroyed);

    m_deviceLinks.append(link);

    //Theoretically we will never add two links from the same provider (the provider should destroy
    //the old one before this is called), so we do not have to worry about destroying old links.
    //-- Actually, we should not destroy them or the provider will store an invalid ref!

    connect(link, &DeviceLink::receivedPackage,
            this, &Device::privateReceivedPackage);

    qSort(m_deviceLinks.begin(), m_deviceLinks.end(), lessThan);

    const bool capabilitiesSupported = identityPackage.has(QStringLiteral("incomingCapabilities")) || identityPackage.has(QStringLiteral("outgoingCapabilities"));
    if (capabilitiesSupported) {
        const QSet<QString> outgoingCapabilities = identityPackage.get<QStringList>(QStringLiteral("outgoingCapabilities")).toSet()
                          , incomingCapabilities = identityPackage.get<QStringList>(QStringLiteral("incomingCapabilities")).toSet();

        m_supportedPlugins = PluginLoader::instance()->pluginsForCapabilities(incomingCapabilities, outgoingCapabilities);
        //qDebug() << "new plugins for" << m_deviceName << m_supportedPlugins << incomingCapabilities << outgoingCapabilities;
    } else {
        m_supportedPlugins = PluginLoader::instance()->getPluginList().toSet();
    }

    reloadPlugins();

    if (m_deviceLinks.size() == 1) {
        Q_EMIT reachableChanged(true);
    }

    connect(link, &DeviceLink::pairStatusChanged, this, &Device::pairStatusChanged);
    connect(link, &DeviceLink::pairingRequest, this, &Device::addPairingRequest);
    connect(link, &DeviceLink::pairingRequestExpired, this, &Device::removePairingRequest);
    connect(link, &DeviceLink::pairingError, this, &Device::pairingError);
}

void Device::addPairingRequest(PairingHandler* handler)
{
    const bool wasEmpty = m_pairRequests.isEmpty();
    m_pairRequests.insert(handler);

    if (wasEmpty != m_pairRequests.isEmpty())
        Q_EMIT hasPairingRequestsChanged(!m_pairRequests.isEmpty());
}

void Device::removePairingRequest(PairingHandler* handler)
{
    const bool wasEmpty = m_pairRequests.isEmpty();
    m_pairRequests.remove(handler);

    if (wasEmpty != m_pairRequests.isEmpty())
        Q_EMIT hasPairingRequestsChanged(!m_pairRequests.isEmpty());
}

bool Device::hasPairingRequests() const
{
    return !m_pairRequests.isEmpty();
}

void Device::acceptPairing()
{
    if (m_pairRequests.isEmpty())
        qWarning() << "no pair requests to accept!";

    //copying because the pairing handler will be removed upon accept
    const auto prCopy = m_pairRequests;
    for (auto ph: prCopy)
        ph->acceptPairing();
}

void Device::rejectPairing()
{
    if (m_pairRequests.isEmpty())
        qWarning() << "no pair requests to reject!";

    //copying because the pairing handler will be removed upon reject
    const auto prCopy = m_pairRequests;
    for (auto ph: prCopy)
        ph->rejectPairing();
}

void Device::linkDestroyed(QObject* o)
{
    removeLink(static_cast<DeviceLink*>(o));
}

void Device::removeLink(DeviceLink* link)
{
    m_deviceLinks.removeAll(link);

    //qCDebug(KDECONNECT_CORE) << "RemoveLink" << m_deviceLinks.size() << "links remaining";

    if (m_deviceLinks.isEmpty()) {
        reloadPlugins();
        Q_EMIT reachableChanged(false);
    }
}

bool Device::sendPackage(NetworkPackage& np)
{
    Q_ASSERT(np.type() != PACKAGE_TYPE_PAIR);
    Q_ASSERT(isTrusted());

    //Maybe we could block here any package that is not an identity or a pairing package to prevent sending non encrypted data
    for (DeviceLink* dl : qAsConst(m_deviceLinks)) {
        if (dl->sendPackage(np)) return true;
    }

    return false;
}

void Device::privateReceivedPackage(const NetworkPackage& np)
{
    Q_ASSERT(np.type() != PACKAGE_TYPE_PAIR);
    if (isTrusted()) {
        const QList<KdeConnectPlugin*> plugins = m_pluginsByIncomingCapability.values(np.type());
        if (plugins.isEmpty()) {
            qWarning() << "discarding unsupported package" << np.type() << "for" << name();
        }
        for (KdeConnectPlugin* plugin : plugins) {
            plugin->receivePackage(np);
        }
    } else {
        qCDebug(KDECONNECT_CORE) << "device" << name() << "not paired, ignoring package" << np.type();
        unpair();
    }

}

bool Device::isTrusted() const
{
    return KdeConnectConfig::instance()->trustedDevices().contains(id());
}

QStringList Device::availableLinks() const
{
    QStringList sl;
    sl.reserve(m_deviceLinks.size());
    for (DeviceLink* dl : qAsConst(m_deviceLinks)) {
        sl.append(dl->provider()->name());
    }
    return sl;
}

void Device::cleanUnneededLinks() {
    if (isTrusted()) {
        return;
    }
    for(int i = 0; i < m_deviceLinks.size(); ) {
        DeviceLink* dl = m_deviceLinks[i];
        if (!dl->linkShouldBeKeptAlive()) {
            dl->deleteLater();
            m_deviceLinks.remove(i);
        } else {
            i++;
        }
    }
}

QHostAddress Device::getLocalIpAddress() const
{
    for (DeviceLink* dl : m_deviceLinks) {
        LanDeviceLink* ldl = dynamic_cast<LanDeviceLink*>(dl);
        if (ldl) {
            return ldl->hostAddress();
        }
    }
    return QHostAddress::Null;
}

Device::DeviceType Device::str2type(const QString& deviceType) {
    if (deviceType == QLatin1String("desktop")) return Desktop;
    if (deviceType == QLatin1String("laptop")) return Laptop;
    if (deviceType == QLatin1String("smartphone") || deviceType == QLatin1String("phone")) return Phone;
    if (deviceType == QLatin1String("tablet")) return Tablet;
    return Unknown;
}

QString Device::type2str(Device::DeviceType deviceType) {
    if (deviceType == Desktop) return QStringLiteral("desktop");
    if (deviceType == Laptop) return QStringLiteral("laptop");
    if (deviceType == Phone) return QStringLiteral("smartphone");
    if (deviceType == Tablet) return QStringLiteral("tablet");
    return QStringLiteral("unknown");
}

QString Device::statusIconName() const
{
    return iconForStatus(isReachable(), isTrusted());
}

QString Device::iconName() const
{

    return iconForStatus(true, false);
}

QString Device::iconForStatus(bool reachable, bool trusted) const
{
    Device::DeviceType deviceType = m_deviceType;
    if (deviceType == Device::Unknown) {
        deviceType = Device::Phone; //Assume phone if we don't know the type
    } else if (deviceType == Device::Desktop) {
        deviceType = Device::Device::Laptop; // We don't have desktop icon yet
    }

    QString status = (reachable? (trusted? QStringLiteral("connected") : QStringLiteral("disconnected")) : QStringLiteral("trusted"));
    QString type = type2str(deviceType);

    return type+status;
}

void Device::setName(const QString& name)
{
    if (m_deviceName != name) {
        m_deviceName = name;
        Q_EMIT nameChanged(name);
    }
}

KdeConnectPlugin* Device::plugin(const QString& pluginName) const
{
    return m_plugins[pluginName];
}

void Device::setPluginEnabled(const QString& pluginName, bool enabled)
{
    KConfigGroup pluginStates = KSharedConfig::openConfig(pluginsConfigFile())->group("Plugins");

    const QString enabledKey = pluginName + QStringLiteral("Enabled");
    pluginStates.writeEntry(enabledKey, enabled);
    reloadPlugins();
}

bool Device::isPluginEnabled(const QString& pluginName) const
{
    const QString enabledKey = pluginName + QStringLiteral("Enabled");
    KConfigGroup pluginStates = KSharedConfig::openConfig(pluginsConfigFile())->group("Plugins");

    return (pluginStates.hasKey(enabledKey) ? pluginStates.readEntry(enabledKey, false)
                                            : PluginLoader::instance()->getPluginInfo(pluginName).isEnabledByDefault());
}

QString Device::encryptionInfo() const
{
    QString result;
    QCryptographicHash::Algorithm digestAlgorithm = QCryptographicHash::Algorithm::Sha1;

    QString localSha1 = QString::fromLatin1(KdeConnectConfig::instance()->certificate().digest(digestAlgorithm).toHex());
    for (int i = 2; i<localSha1.size(); i += 3) {
        localSha1.insert(i, ':'); // Improve readability
    }
    result += i18n("SHA1 fingerprint of your device certificate is: %1\n", localSha1);

    std::string  remotePem = KdeConnectConfig::instance()->getDeviceProperty(id(), QStringLiteral("certificate")).toStdString();
    QSslCertificate remoteCertificate = QSslCertificate(QByteArray(remotePem.c_str(), (int)remotePem.size()));
    QString remoteSha1 = QString::fromLatin1(remoteCertificate.digest(digestAlgorithm).toHex());
    for (int i = 2; i < remoteSha1.size(); i += 3) {
        remoteSha1.insert(i, ':'); // Improve readability
    }
    result += i18n("SHA1 fingerprint of remote device certificate is: %1\n", remoteSha1);

    return result;
}

