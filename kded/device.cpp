#include "device.h"

#include <KSharedPtr>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KStandardDirs>
#include <KPluginSelector>
#include <KServiceTypeTrader>
#include <KPluginInfo>
#include <KNotification>
#include <KIcon>

#include <QDBusConnection>
#include <QDebug>

#include "plugins/kdeconnectplugin.h"
#include "plugins/pluginloader.h"
#include "backends/devicelink.h"
#include "backends/linkprovider.h"
#include "networkpackage.h"

Device::Device(const QString& id)
    : m_deviceId(id)
    , m_pairStatus(Device::Paired)
    , m_protocolVersion(NetworkPackage::ProtocolVersion) //We don't know it yet
{
    KSharedConfigPtr config = KSharedConfig::openConfig("kdeconnectrc");
    KConfigGroup data = config->group("trusted_devices").group(id);

    m_deviceName = data.readEntry<QString>("deviceName", QString("unnamed"));

    const QString& key = data.readEntry<QString>("publicKey",QString());
    m_publicKey = QCA::RSAPublicKey::fromPEM(key);

    //Register in bus
    QDBusConnection::sessionBus().registerObject(dbusPath(), this, QDBusConnection::ExportScriptableContents | QDBusConnection::ExportAdaptors);

}

Device::Device(const NetworkPackage& identityPackage, DeviceLink* dl)
    : m_deviceId(identityPackage.get<QString>("deviceId"))
    , m_deviceName(identityPackage.get<QString>("deviceName"))
    , m_pairStatus(Device::NotPaired)
    , m_protocolVersion(identityPackage.get<int>("protocolVersion"))
{
    addLink(identityPackage, dl);

    //Register in bus
    QDBusConnection::sessionBus().registerObject(dbusPath(), this, QDBusConnection::ExportScriptableContents | QDBusConnection::ExportAdaptors);
}

Device::~Device()
{

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
    QMap< QString, KdeConnectPlugin* > newPluginMap;

    if (isPaired() && isReachable()) { //Do not load any plugin for unpaired devices, nor useless loading them for unreachable devices

        QString path = KGlobal::dirs()->findResource("config", "kdeconnect/"+id());
        KConfigGroup pluginStates = KSharedConfig::openConfig(path)->group("Plugins");

        PluginLoader* loader = PluginLoader::instance();

        //Code borrowed from KWin
        foreach (const QString& pluginName, loader->getPluginList()) {
            QString enabledKey = pluginName + QString::fromLatin1("Enabled");

            bool isPluginEnabled = (pluginStates.hasKey(enabledKey) ? pluginStates.readEntry(enabledKey, false)
                                                            : loader->getPluginInfo(pluginName).isPluginEnabledByDefault());

            if (isPluginEnabled) {
                KdeConnectPlugin* reusedPluginInstance = m_plugins.take(pluginName);
                if (reusedPluginInstance) {
                    //Already loaded, reuse it
                    newPluginMap[pluginName] = reusedPluginInstance;
                } else {
                    KdeConnectPlugin* plugin = loader->instantiatePluginForDevice(pluginName, this);

                    connect(this, SIGNAL(receivedPackage(NetworkPackage)),
                            plugin, SLOT(receivePackage(NetworkPackage)));

                    newPluginMap[pluginName] = plugin;
                }
            }
        }
    }

    //Erase all left plugins in the original map (meaning that we don't want
    //them anymore, otherwise they would have been moved to the newPluginMap)
    qDeleteAll(m_plugins);
    m_plugins = newPluginMap;

    Q_FOREACH(KdeConnectPlugin* plugin, m_plugins) {
        plugin->connected();
    }

    Q_EMIT pluginsChanged();

}

void Device::requestPair()
{
    switch(m_pairStatus) {
        case Device::Paired:
            Q_EMIT pairingFailed(i18n("Already paired"));
            return;
        case Device::Requested:
            Q_EMIT pairingFailed(i18n("Pairing already requested for this device"));
            return;
        default:
            if (!isReachable()) {
                Q_EMIT pairingFailed(i18n("Device not reachable"));
                return;
            }
            break;
    }

    m_pairStatus = Device::Requested;

    //Send our own public key
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", true);
    KSharedConfigPtr config = KSharedConfig::openConfig("kdeconnectrc");
    const QString& key = config->group("myself").readEntry<QString>("publicKey",QString());
    np.set("publicKey",key);
    bool success = sendPackage(np);

    if (!success) {
        m_pairStatus = Device::NotPaired;
        Q_EMIT pairingFailed(i18n("Error contacting device"));
        return;
    }

    if (m_pairStatus == Device::Paired) return;
    pairingTimer.setSingleShot(true);
    pairingTimer.start(20 * 1000);
    connect(&pairingTimer, SIGNAL(timeout()),
            this, SLOT(pairingTimeout()));

}

void Device::unpair()
{
    if (!isPaired()) return;

    m_pairStatus = Device::NotPaired;

    KSharedConfigPtr config = KSharedConfig::openConfig("kdeconnectrc");
    config->group("trusted_devices").deleteGroup(id());

    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", false);
    sendPackage(np);

    reloadPlugins(); //Will unload the plugins

    Q_EMIT unpaired();

}

void Device::pairingTimeout()
{
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", false);
    sendPackage(np);
    m_pairStatus = Device::NotPaired;
    Q_EMIT pairingFailed(i18n("Timed out"));
}

static bool lessThan(DeviceLink* p1, DeviceLink* p2)
{
    return p1->provider()->priority() > p2->provider()->priority();
}

void Device::addLink(const NetworkPackage& identityPackage, DeviceLink* link)
{
    //qDebug() << "Adding link to" << id() << "via" << link->provider();

    m_protocolVersion = identityPackage.get<int>("protocolVersion");
    if (m_protocolVersion != NetworkPackage::ProtocolVersion) {
        qWarning() << m_deviceName << "- warning, device uses a different protocol version" << m_protocolVersion << "expected" << NetworkPackage::ProtocolVersion;
    }

    connect(link, SIGNAL(destroyed(QObject*)),
            this, SLOT(linkDestroyed(QObject*)));

    m_deviceLinks.append(link);

    //TODO: Do not read the key every time
    KSharedConfigPtr config = KSharedConfig::openConfig("kdeconnectrc");
    const QString& key = config->group("myself").readEntry<QString>("privateKey",QString());
    QCA::PrivateKey privateKey = QCA::PrivateKey::fromPEM(key);
    link->setPrivateKey(privateKey);

    //Theoretically we will never add two links from the same provider (the provider should destroy
    //the old one before this is called), so we do not have to worry about destroying old links.
    //Actually, we should not destroy them or the provider will store an invalid ref!

    connect(link, SIGNAL(receivedPackage(NetworkPackage)),
            this, SLOT(privateReceivedPackage(NetworkPackage)));

    qSort(m_deviceLinks.begin(), m_deviceLinks.end(), lessThan);

    if (m_deviceLinks.size() == 1) {
        reloadPlugins(); //Will load the plugins
        Q_EMIT reachableStatusChanged();
    } else {
        Q_FOREACH(KdeConnectPlugin* plugin, m_plugins) {
            plugin->connected();
        }
    }
}

void Device::linkDestroyed(QObject* o)
{
    removeLink(static_cast<DeviceLink*>(o));
}

void Device::removeLink(DeviceLink* link)
{
    m_deviceLinks.removeOne(link);

    //qDebug() << "RemoveLink" << m_deviceLinks.size() << "links remaining";

    if (m_deviceLinks.isEmpty()) {
        reloadPlugins();
        Q_EMIT reachableStatusChanged();
    }
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

        //qDebug() << "Pair package";

        bool wantsPair = np.get<bool>("pair");

        if (wantsPair == isPaired()) {
            qDebug() << "Already" << (wantsPair? "paired":"unpaired");
            if (m_pairStatus == Device::Requested) {
                m_pairStatus = Device::NotPaired;
                pairingTimer.stop();
                Q_EMIT pairingFailed(i18n("Canceled by other peer"));
            }
            return;
        }

        if (wantsPair) {

            //Retrieve their public key
            const QString& key = np.get<QString>("publicKey");
            m_publicKey = QCA::RSAPublicKey::fromPEM(key);
            if (m_publicKey.isNull()) {
                qDebug() << "ERROR decoding key";
                if (m_pairStatus == Device::Requested) {
                    m_pairStatus = Device::NotPaired;
                    pairingTimer.stop();
                }
                Q_EMIT pairingFailed(i18n("Received incorrect key"));
                return;
            }

            if (m_pairStatus == Device::Requested)  { //We started pairing

                qDebug() << "Pair answer";

                m_pairStatus = Device::Paired;
                pairingTimer.stop();

                //Store as trusted device
                KSharedConfigPtr config = KSharedConfig::openConfig("kdeconnectrc");
                config->group("trusted_devices").group(id()).writeEntry("publicKey",key);
                config->group("trusted_devices").group(id()).writeEntry("deviceName",name());

                reloadPlugins();

                Q_EMIT pairingSuccesful();

            } else {

                qDebug() << "Pair request";

                KNotification* notification = new KNotification("pingReceived"); //KNotification::Persistent
                notification->setPixmap(KIcon("dialog-information").pixmap(48, 48));
                notification->setComponentData(KComponentData("kdeconnect", "kdeconnect"));
                notification->setTitle("KDE Connect");
                notification->setText(i18n("Pairing request from %1", m_deviceName));
                notification->setActions(QStringList() << i18n("Accept") << i18n("Reject"));
                connect(notification, SIGNAL(action1Activated()), this, SLOT(acceptPairing()));
                connect(notification, SIGNAL(action2Activated()), this, SLOT(rejectPairing()));
                notification->sendEvent();

                m_pairStatus = Device::RequestedByPeer;

            }

        } else {

            qDebug() << "Unpair request";

            if (m_pairStatus == Device::Requested) {
                pairingTimer.stop();
                Q_EMIT pairingFailed(i18n("Canceled by other peer"));
            } else if (m_pairStatus == Device::Paired) {
                KSharedConfigPtr config = KSharedConfig::openConfig("kdeconnectrc");
                config->group("trusted_devices").deleteGroup(id());
                reloadPlugins();
                Q_EMIT unpaired();
            }

            m_pairStatus = Device::NotPaired;

        }

    } else if (!isPaired()) {

        //TODO: Notify the other side that we don't trust them
        qDebug() << "device" << name() << "not paired, ignoring package" << np.type();

    } else {

        //Forward package
        Q_EMIT receivedPackage(np);

    }

}

void Device::acceptPairing()
{
    if (m_pairStatus != Device::RequestedByPeer) return;

    qDebug() << "Accepted pairing";

    KSharedConfigPtr config = KSharedConfig::openConfig("kdeconnectrc");

    //Send our own public key
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", true);
    const QString& key = config->group("myself").readEntry<QString>("publicKey",QString());
    np.set("publicKey",key);
    bool success = sendPackage(np);

    if (!success) {
        return;
    }

    //Store as trusted device
    config->group("trusted_devices").group(id()).writeEntry("publicKey", m_publicKey.toPEM());
    config->group("trusted_devices").group(id()).writeEntry("deviceName", name());

    m_pairStatus = Device::Paired;

    reloadPlugins(); //This will load plugins

    Q_EMIT pairingSuccesful();
}

void Device::rejectPairing()
{
    qDebug() << "Rejected pairing";

    m_pairStatus = Device::NotPaired;

    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", false);
    sendPackage(np);

    Q_EMIT pairingFailed(i18n("Canceled by the user"));

}

QStringList Device::availableLinks() const
{
    QStringList sl;
    Q_FOREACH(DeviceLink* dl, m_deviceLinks) {
        sl.append(dl->provider()->name());
    }
    return sl;
}

void Device::sendPing()
{
    NetworkPackage np("kdeconnect.ping");
    bool success = sendPackage(np);
    qDebug() << "sendPing:" << success;
}

