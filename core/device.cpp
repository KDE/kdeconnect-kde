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
#include <QFile>
#include <QStandardPaths>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <QIcon>
#include <QDir>
#include <QJsonArray>

#include "core_debug.h"
#include "kdeconnectplugin.h"
#include "pluginloader.h"
#include "backends/devicelink.h"
#include "backends/linkprovider.h"
#include "networkpackage.h"
#include "kdeconnectconfig.h"
#include "daemon.h"

Q_LOGGING_CATEGORY(KDECONNECT_CORE, "kdeconnect.core")

Device::Device(QObject* parent, const QString& id)
    : QObject(parent)
    , m_deviceId(id)
    , m_pairStatus(Device::Paired)
    , m_protocolVersion(NetworkPackage::ProtocolVersion) //We don't know it yet
{
    KdeConnectConfig::DeviceInfo info = KdeConnectConfig::instance()->getTrustedDevice(id);

    m_deviceName = info.deviceName;
    m_deviceType = str2type(info.deviceType);
    m_publicKey = QCA::RSAPublicKey::fromPEM(info.publicKey);

    //Register in bus
    QDBusConnection::sessionBus().registerObject(dbusPath(), this, QDBusConnection::ExportScriptableContents | QDBusConnection::ExportAdaptors);
}

Device::Device(QObject* parent, const NetworkPackage& identityPackage, DeviceLink* dl)
    : QObject(parent)
    , m_deviceId(identityPackage.get<QString>("deviceId"))
    , m_deviceName(identityPackage.get<QString>("deviceName"))
    , m_deviceType(str2type(identityPackage.get<QString>("deviceType")))
    , m_pairStatus(Device::NotPaired)
    , m_protocolVersion(identityPackage.get<int>("protocolVersion", -1))
{
    addLink(identityPackage, dl);

    //Register in bus
    QDBusConnection::sessionBus().registerObject(dbusPath(), this, QDBusConnection::ExportScriptableContents | QDBusConnection::ExportAdaptors);

    //Implement deprecated signals
    connect(this, &Device::pairingChanged, this, [this](bool newPairing) {
        if (newPairing)
            Q_EMIT pairingSuccesful();
        else
            Q_EMIT unpaired();
    });
}

Device::~Device()
{
    qDeleteAll(m_pairingHandlers);
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
    QHash<QString, KdeConnectPlugin*> newPluginMap;
    QMultiMap<QString, KdeConnectPlugin*> newPluginsByIncomingInterface;
    QMultiMap<QString, KdeConnectPlugin*> newPluginsByOutgoingInterface;
    QSet<QString> supportedIncomingInterfaces;
    QSet<QString> supportedOutgoingInterfaces;
    QStringList unsupportedPlugins;

    if (isPaired() && isReachable()) { //Do not load any plugin for unpaired devices, nor useless loading them for unreachable devices

        KConfigGroup pluginStates = KSharedConfig::openConfig(pluginsConfigFile())->group("Plugins");

        PluginLoader* loader = PluginLoader::instance();
        const bool deviceSupportsCapabilities = !m_incomingCapabilities.isEmpty() || !m_outgoingCapabilities.isEmpty();

        foreach (const QString& pluginName, loader->getPluginList()) {
            const KPluginMetaData service = loader->getPluginInfo(pluginName);
            const QSet<QString> incomingInterfaces = KPluginMetaData::readStringList(service.rawData(), "X-KdeConnect-SupportedPackageType").toSet();
            const QSet<QString> outgoingInterfaces = KPluginMetaData::readStringList(service.rawData(), "X-KdeConnect-OutgoingPackageType").toSet();

            const bool pluginEnabled = isPluginEnabled(pluginName);

            if (pluginEnabled) {
                supportedIncomingInterfaces += incomingInterfaces;
                supportedOutgoingInterfaces += outgoingInterfaces;
            }

            //If we don't find intersection with the received on one end and the sent on the other, we don't
            //let the plugin stay
            //Also, if no capabilities are specified on the other end, we don't apply this optimizaton, as
            //we assume that the other client doesn't know about capabilities.
            const bool capabilitiesSupported = deviceSupportsCapabilities && (!incomingInterfaces.isEmpty() || !outgoingInterfaces.isEmpty());
            if (capabilitiesSupported
                && (m_incomingCapabilities & outgoingInterfaces).isEmpty()
                && (m_outgoingCapabilities & incomingInterfaces).isEmpty()
            ) {
                qCWarning(KDECONNECT_CORE) << "not loading " << pluginName << "because of unmatched capabilities";
                unsupportedPlugins.append(pluginName);
                continue;
            }

            if (pluginEnabled) {
                KdeConnectPlugin* plugin = m_plugins.take(pluginName);

                if (!plugin) {
                    plugin = loader->instantiatePluginForDevice(pluginName, this);
                }

                foreach(const QString& interface, incomingInterfaces) {
                    newPluginsByIncomingInterface.insert(interface, plugin);
                }
                foreach(const QString& interface, outgoingInterfaces) {
                    newPluginsByOutgoingInterface.insert(interface, plugin);
                }

                newPluginMap[pluginName] = plugin;
            }
        }
    }

    //Erase all left plugins in the original map (meaning that we don't want
    //them anymore, otherwise they would have been moved to the newPluginMap)
    const QStringList newSupportedIncomingInterfaces = supportedIncomingInterfaces.toList();
    const QStringList newSupportedOutgoingInterfaces = supportedOutgoingInterfaces.toList();
    const bool capabilitiesChanged = (m_pluginsByOutgoingInterface != newPluginsByOutgoingInterface
                                     || m_supportedIncomingInterfaces != newSupportedIncomingInterfaces);
    qDeleteAll(m_plugins);
    m_plugins = newPluginMap;
    m_supportedIncomingInterfaces = newSupportedIncomingInterfaces;
    m_supportedOutgoingInterfaces = newSupportedOutgoingInterfaces;
    m_pluginsByOutgoingInterface = newPluginsByOutgoingInterface;
    m_pluginsByIncomingInterface = newPluginsByIncomingInterface;
    m_unsupportedPlugins = unsupportedPlugins;

    Q_FOREACH(KdeConnectPlugin* plugin, m_plugins) {
        plugin->connected();
    }
    Q_EMIT pluginsChanged();

    if (capabilitiesChanged && isReachable() && isPaired())
    {
        NetworkPackage np(PACKAGE_TYPE_CAPABILITIES);
        np.set<QStringList>("IncomingCapabilities", newSupportedIncomingInterfaces);
        np.set<QStringList>("OutgoingCapabilities", newSupportedOutgoingInterfaces);
        sendPackage(np);
    }
}

QString Device::pluginsConfigFile() const
{
    return KdeConnectConfig::instance()->deviceConfigDir(id()).absoluteFilePath("config");
}

bool Device::pairRequested() const
{
    bool isPairRequested = false;
    Q_FOREACH(PairingHandler* ph, m_pairingHandlers) {
        isPairRequested = isPairRequested || ph->pairRequested();
    }
    return isPairRequested;
}

void Device::requestPair()
{
    switch(m_pairStatus) {
        case Device::Paired:
            Q_EMIT pairingFailed(i18n("Already paired"));
            return;
        case Device::NotPaired:
            ;
    }

    if (!isReachable()) {
        Q_EMIT pairingFailed(i18n("Device not reachable"));
        return;
    }

    //Send our own public key
    bool success = false;
    Q_FOREACH(PairingHandler* ph, m_pairingHandlers.values()) {
        success = success || ph->requestPairing(); // If one of many pairing handlers can successfully request pairing, consider it success
    }
    if (!success) {
        m_pairStatus = Device::NotPaired;
        Q_EMIT pairingFailed(i18n("Error contacting device"));
        return;
    }

    if (m_pairStatus == Device::Paired) {
        return;
    }
}

void Device::unpair()
{
    Q_FOREACH(PairingHandler* ph, m_pairingHandlers.values()) {
        ph->unpair();
    }
    unpairInternal(); // We do not depend on pairing handlers for unpairing, this method is likely to be called due to user events
}

void Device::unpairInternal()
{
    bool alreadyUnpaired = (m_pairStatus != Device::Paired);
    m_pairStatus = Device::NotPaired;
    KdeConnectConfig::instance()->removeTrustedDevice(id());
    reloadPlugins(); //Will unload the plugins
    if (!alreadyUnpaired) {
        Q_EMIT pairingChanged(false);
    }
}

void Device::pairingTimeout()
{
    Q_EMIT pairingFailed(i18n("Timed out"));
}

static bool lessThan(DeviceLink* p1, DeviceLink* p2)
{
    return p1->provider()->priority() > p2->provider()->priority();
}

void Device::addLink(const NetworkPackage& identityPackage, DeviceLink* link)
{
    //qCDebug(KDECONNECT_CORE) << "Adding link to" << id() << "via" << link->provider();

    m_protocolVersion = identityPackage.get<int>("protocolVersion", -1);
    if (m_protocolVersion != NetworkPackage::ProtocolVersion) {
        qCWarning(KDECONNECT_CORE) << m_deviceName << "- warning, device uses a different protocol version" << m_protocolVersion << "expected" << NetworkPackage::ProtocolVersion;
    }

    connect(link, SIGNAL(destroyed(QObject*)),
            this, SLOT(linkDestroyed(QObject*)));

    m_deviceLinks.append(link);

    //re-read the device name from the identityPackage because it could have changed
    setName(identityPackage.get<QString>("deviceName"));
    m_deviceType = str2type(identityPackage.get<QString>("deviceType"));

    // Set certificate if the link is on ssl, and it is added to identity package
    // This is always sets certificate when link is added to device
    if (identityPackage.has("certificate")) {
        m_certificate = QSslCertificate(identityPackage.get<QByteArray>("certificate"));
    }

    //Theoretically we will never add two links from the same provider (the provider should destroy
    //the old one before this is called), so we do not have to worry about destroying old links.
    //-- Actually, we should not destroy them or the provider will store an invalid ref!

    connect(link, SIGNAL(receivedPackage(NetworkPackage)),
            this, SLOT(privateReceivedPackage(NetworkPackage)));

    qSort(m_deviceLinks.begin(), m_deviceLinks.end(), lessThan);

    if (m_deviceLinks.size() == 1) {
        m_incomingCapabilities = identityPackage.get<QStringList>("IncomingCapabilities", QStringList()).toSet();
        m_outgoingCapabilities = identityPackage.get<QStringList>("OutgoingCapabilities", QStringList()).toSet();
        reloadPlugins(); //Will load the plugins
        Q_EMIT reachableStatusChanged();
    } else {
        Q_FOREACH(KdeConnectPlugin* plugin, m_plugins) {
            plugin->connected();
        }
    }

    // One pairing handler per type of device link, so check if there is already a pairing handler of same type, if not add it to the list
    if (!m_pairingHandlers.contains(link->name())) {
        PairingHandler* pairingHandler = link->createPairingHandler(this);
        m_pairingHandlers.insert(link->name(), pairingHandler);
        connect(m_pairingHandlers[link->name()], SIGNAL(linkNull()), this, SLOT(destroyPairingHandler()));
        connect(m_pairingHandlers[link->name()], SIGNAL(pairingDone()), this, SLOT(setAsPaired()));
        connect(m_pairingHandlers[link->name()], SIGNAL(unpairingDone()), this, SLOT(unpairInternal()));
        connect(m_pairingHandlers[link->name()], SIGNAL(pairingFailed(const QString&)), this, SIGNAL(pairingFailed(const QString&)));
    }
    m_pairingHandlers[link->name()]->setLink(link);
    connect(link, SIGNAL(destroyed(QObject*)), m_pairingHandlers[link->name()], SLOT(linkDestroyed(QObject*)));
}

void Device::linkDestroyed(QObject* o)
{
    removeLink(static_cast<DeviceLink*>(o));
}

void Device::removeLink(DeviceLink* link)
{
    m_deviceLinks.removeOne(link);

    //qCDebug(KDECONNECT_CORE) << "RemoveLink" << m_deviceLinks.size() << "links remaining";

    if (m_deviceLinks.isEmpty()) {
        reloadPlugins();
        Q_EMIT reachableStatusChanged();
    }
}

void Device::destroyPairingHandler()
{
    PairingHandler* ph = qobject_cast<PairingHandler*>(sender());
    m_pairingHandlers.remove(m_pairingHandlers.key(ph));
    delete ph;
}

bool Device::sendPackage(NetworkPackage& np)
{
    if (np.type() != PACKAGE_TYPE_PAIR && isPaired()) {
        Q_FOREACH(DeviceLink* dl, m_deviceLinks) {
            if (dl->sendPackageEncrypted(m_publicKey, np)) return true;
        }
    } else {
        //Maybe we could block here any package that is not an identity or a pairing package to prevent sending non encrypted data
        Q_FOREACH(DeviceLink* dl, m_deviceLinks) {
            if (dl->sendPackage(np)) return true;
        }
    }

    return false;
}

void Device::privateReceivedPackage(const NetworkPackage& np)
{
    if (np.type() == PACKAGE_TYPE_PAIR) {

        // If PACKAGE_TYPE_PAIR, send it to pairing handlers without thinking
        Q_FOREACH(PairingHandler* ph, m_pairingHandlers) {
            ph->packageReceived(np);
        }

    } else if (np.type() == PACKAGE_TYPE_CAPABILITIES) {
        QSet<QString> newIncomingCapabilities = np.get<QStringList>("IncomingCapabilities", QStringList()).toSet();
        QSet<QString> newOutgoingCapabilities = np.get<QStringList>("OutgoingCapabilities", QStringList()).toSet();

        if (newOutgoingCapabilities != m_outgoingCapabilities || newIncomingCapabilities != m_incomingCapabilities) {
            m_incomingCapabilities = newIncomingCapabilities;
            m_outgoingCapabilities = newOutgoingCapabilities;
            reloadPlugins();
        }
    } else if (isPaired()) {
        QList<KdeConnectPlugin*> plugins = m_pluginsByIncomingInterface.values(np.type());
        foreach(KdeConnectPlugin* plugin, plugins) {
            plugin->receivePackage(np);
        }
    } else {
        qCDebug(KDECONNECT_CORE) << "device" << name() << "not paired, ignoring package" << np.type();
        unpair();
    }

}

void Device::rejectPairing()
{
    qCDebug(KDECONNECT_CORE) << "Rejected pairing";

    m_pairStatus = Device::NotPaired;

    Q_FOREACH(PairingHandler* ph, m_pairingHandlers.values()) {
            ph->rejectPairing();
    }

    Q_EMIT pairingFailed(i18n("Canceled by the user"));

}

void Device::acceptPairing()
{
    qCDebug(KDECONNECT_CORE) << "Accepted pairing";

    bool success = false;
    Q_FOREACH(PairingHandler* ph, m_pairingHandlers.values()) {
            success = success || ph->acceptPairing();
    }

    if (!success) {
        m_pairStatus = Device::NotPaired;
        return;
    }

}

void Device::setAsPaired()
{

    bool alreadyPaired = (m_pairStatus == Device::Paired);

    m_pairStatus = Device::Paired;

    //Save device info in the config
    KdeConnectConfig::instance()->addTrustedDevice(id(), name(), type2str(m_deviceType));

    reloadPlugins(); //Will actually load the plugins

    if (!alreadyPaired) {
        Q_EMIT pairingChanged(true);
    }

}

DeviceLink::ConnectionStarted Device::connectionSource() const
{
    DeviceLink::ConnectionStarted ret = DeviceLink::Remotely;
    Q_FOREACH(DeviceLink* link, m_deviceLinks) {
        if(link->connectionSource() == DeviceLink::ConnectionStarted::Locally) {
            ret = DeviceLink::ConnectionStarted::Locally;
            break;
        }
    }
    return ret;
}

QStringList Device::availableLinks() const
{
    QStringList sl;
    Q_FOREACH(DeviceLink* dl, m_deviceLinks) {
        sl.append(dl->provider()->name());
    }
    return sl;
}

Device::DeviceType Device::str2type(const QString &deviceType) {
    if (deviceType == "desktop") return Desktop;
    if (deviceType == "laptop") return Laptop;
    if (deviceType == "smartphone" || deviceType == "phone") return Phone;
    if (deviceType == "tablet") return Tablet;
    return Unknown;
}

QString Device::type2str(Device::DeviceType deviceType) {
    if (deviceType == Desktop) return "desktop";
    if (deviceType == Laptop) return "laptop";
    if (deviceType == Phone) return "smartphone";
    if (deviceType == Tablet) return "tablet";
    return "unknown";
}

QString Device::statusIconName() const
{
    return iconForStatus(isReachable(), isPaired());
}

QString Device::iconName() const
{

    return iconForStatus(true, false);
}

QString Device::iconForStatus(bool reachable, bool paired) const
{
    Device::DeviceType deviceType = m_deviceType;
    if (deviceType == Device::Unknown) {
        deviceType = Device::Phone; //Assume phone if we don't know the type
    } else if (deviceType == Device::Desktop) {
        deviceType = Device::Device::Laptop; // We don't have desktop icon yet
    }

    QString status = (reachable? (paired? QStringLiteral("connected") : QStringLiteral("disconnected")) : QStringLiteral("trusted"));
    QString type = type2str(deviceType);

    return type+'-'+status;
}

void Device::setName(const QString &name)
{
    if (m_deviceName != name) {
        m_deviceName = name;
        Q_EMIT nameChanged(name);
    }
}

Device::PairStatus Device::pairStatus() const
{
    return m_pairStatus;
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
