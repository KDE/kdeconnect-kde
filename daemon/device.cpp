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

#include <QSslKey>
#include <QDebug>

#include "plugins/kdeconnectplugin.h"
#include "plugins/pluginloader.h"
#include "devicelinks/devicelink.h"
#include "linkproviders/linkprovider.h"
#include "networkpackage.h"

Device::Device(const QString& id)
{

    m_deviceId = id;

    KSharedConfigPtr config = KSharedConfig::openConfig("kdeconnectrc");
    const KConfigGroup& data = config->group("devices").group(id);

    const QString& name = data.readEntry<QString>("name", QString("unnamed"));
    m_deviceName = name;

    const QByteArray& key = data.readEntry<QByteArray>("publicKey",QByteArray());
    m_publicKey = QSslKey(key, QSsl::Rsa, QSsl::Pem, QSsl::PublicKey);

    m_pairingRequested = false;

    //Register in bus
    QDBusConnection::sessionBus().registerObject(dbusPath(), this, QDBusConnection::ExportScriptableContents | QDBusConnection::ExportAdaptors);

}

Device::Device(const NetworkPackage& identityPackage, DeviceLink* dl)
{
    m_deviceId = identityPackage.get<QString>("deviceId");
    m_deviceName = identityPackage.get<QString>("deviceName");

    addLink(dl);

    m_pairingRequested = false;

    //Register in bus
    QDBusConnection::sessionBus().registerObject(dbusPath(), this, QDBusConnection::ExportScriptableContents | QDBusConnection::ExportAdaptors);
}

Device::~Device()
{

}

bool Device::hasPlugin(const QString& name)
{
    return m_plugins.contains(name);
}

QStringList Device::loadedPlugins()
{
    return m_plugins.keys();
}

void Device::reloadPlugins()
{
    QMap< QString, KdeConnectPlugin* > newPluginMap;

    if (isPaired() && reachable()) { //Do not load any plugin for unpaired devices, nor useless loading them for unreachable devices

        QString path = KStandardDirs().resourceDirs("config").first()+"kdeconnect/";
        QMap<QString,QString> pluginStates = KSharedConfig::openConfig(path + id())->group("Plugins").entryMap();

        PluginLoader* loader = PluginLoader::instance();

        //Code borrowed from KWin
        foreach (const QString& pluginName, loader->getPluginList()) {

            const QString value = pluginStates.value(pluginName + QString::fromLatin1("Enabled"), QString());
            KPluginInfo info = loader->getPluginInfo(pluginName);
            bool enabled = (value.isNull() ? info.isPluginEnabledByDefault() : QVariant(value).toBool());

            if (enabled) {

                if (m_plugins.contains(pluginName)) {
                    //Already loaded, reuse it
                    newPluginMap[pluginName] = m_plugins[pluginName];
                    m_plugins.remove(pluginName);
                } else {
                    KdeConnectPlugin* plugin = loader->instantiatePluginForDevice(pluginName, this);

                    connect(this, SIGNAL(receivedPackage(const NetworkPackage&)),
                            plugin, SLOT(receivePackage(const NetworkPackage&)));

                    newPluginMap[pluginName] = plugin;
                }
            }
        }
    }

    //Erase all the plugins left in the original map (it means that we don't want
    //them anymore, otherways they would have been moved to the newPluginMap)
    qDeleteAll(m_plugins);
    m_plugins.clear();

    m_plugins = newPluginMap;

    Q_FOREACH(KdeConnectPlugin* plugin, m_plugins) {
            plugin->connected();
    }

    Q_EMIT pluginsChanged();

}

//TODO
QSslKey myPrivateKey() {

    KSharedConfigPtr config = KSharedConfig::openConfig("kdeconnectrc");
    const QString& key = config->group("myself").readEntry<QString>("privateKey",QString());

    QSslKey privateKey(key.toAscii(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
    qDebug() << "Valid public key:" << !privateKey.isNull();

    return privateKey;

}

void Device::requestPair()
{
    if (isPaired() || m_pairingRequested) {
        Q_EMIT pairingFailed(i18n("Already paired"));
        return;
    }
    if (!reachable()) {
        Q_EMIT pairingFailed(i18n("Device not reachable"));
        return;
    }

    //Send our own public key
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", true);
    KSharedConfigPtr config = KSharedConfig::openConfig("kdeconnectrc");
    const QByteArray& key = config->group("myself").readEntry<QByteArray>("publicKey",QByteArray());
    np.set("publicKey",key);
    bool success = sendPackage(np);

    if (!success) {
        Q_EMIT pairingFailed(i18n("Error contacting device"));
        return;
    }

    m_pairingRequested = true;
    pairingTimer.start(20 * 1000);
    connect(&pairingTimer, SIGNAL(timeout()),
            this, SLOT(pairingTimeout()));

}

void Device::unpair()
{
    if (!isPaired()) return;

    m_publicKey = QSslKey();
    m_pairingRequested = false;
    pairingTimer.stop();

    KSharedConfigPtr config = KSharedConfig::openConfig("kdeconnectrc");
    config->group("devices").deleteGroup(id());

    if (reachable()) {
        NetworkPackage np(PACKAGE_TYPE_PAIR);
        np.set("pair", false);
        sendPackage(np);
    }

    reloadPlugins(); //Will unload the plugins

}

void Device::pairingTimeout()
{
    m_pairingRequested = false;
    Q_EMIT Q_EMIT pairingFailed("Timed out");
}

static bool lessThan(DeviceLink* p1, DeviceLink* p2)
{
    return p1->provider()->priority() > p2->provider()->priority();
}

void Device::addLink(DeviceLink* link)
{
    qDebug() << "Adding link to" << id() << "via" << link->provider();

    connect(link, SIGNAL(destroyed(QObject*)),
            this, SLOT(linkDestroyed(QObject*)));

    m_deviceLinks.append(link);

    //Theoretically we will never add two links from the same provider (the provider should destroy
    //the old one before this is called), so we do not have to worry about destroying old links.
    //Actually, we should not destroy them or the provider will store an invalid ref!

    connect(link, SIGNAL(receivedPackage(NetworkPackage)), this, SLOT(privateReceivedPackage(NetworkPackage)));

    qSort(m_deviceLinks.begin(),m_deviceLinks.end(),lessThan);

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

    qDebug() << "RemoveLink" << m_deviceLinks.size() << "links remaining";

    if (m_deviceLinks.empty()) {
        reloadPlugins();
        Q_EMIT reachableStatusChanged();
    }
}

bool Device::sendPackage(const NetworkPackage& np) const
{
    //Maybe we could block here any package that is not an identity or a pairing package

    if (isPaired()) {

    }

    Q_FOREACH(DeviceLink* dl, m_deviceLinks) {
        //TODO: Actually detect if a package is received or not, now we keep TCP
        //"ESTABLISHED" connections that look legit (return true when we use them),
        //but that are actually broken
        if (dl->sendPackage(np)) return true;
    }
    return false;
}

void Device::privateReceivedPackage(const NetworkPackage& np)
{

    if (np.type() == PACKAGE_TYPE_PAIR) {

        qDebug() << "Pair package";

        bool pair = np.get<bool>("pair");
        if (pair == isPaired()) {
            qDebug() << "Already" << (pair? "paired":"unpaired");
            return;
        }

        KSharedConfigPtr config = KSharedConfig::openConfig("kdeconnectrc");

        if (pair) {

            if (m_pairingRequested)  { //We started pairing

                qDebug() << "Pair answer";

                m_pairingRequested = false;
                pairingTimer.stop();

                //Store as trusted device
                const QByteArray& key = np.get<QByteArray>("publicKey");
                config->group("devices").group(id()).writeEntry("publicKey",key);
                config->group("devices").group(id()).writeEntry("name",name());
                m_publicKey = QSslKey(key, QSsl::Rsa, QSsl::Pem, QSsl::PublicKey);

                Q_EMIT pairingSuccesful();

            } else {

                qDebug() << "Pair request";

                const QString& key = np.get<QString>("publicKey");
                m_tempPublicKey = QSslKey(key.toAscii(), QSsl::Rsa, QSsl::Pem, QSsl::PublicKey);

                KNotification* notification = new KNotification("pingReceived"); //KNotification::Persistent
                notification->setPixmap(KIcon("dialog-information").pixmap(48, 48));
                notification->setComponentData(KComponentData("kdeconnect", "kdeconnect"));
                notification->setTitle("KDE Connect");
                notification->setText(i18n("Pairing request from %1", m_deviceName));
                notification->setActions(QStringList() << i18n("Accept") << i18n("Reject"));
                connect(notification, SIGNAL(action1Activated()), this, SLOT(acceptPairing()));
                connect(notification, SIGNAL(action2Activated()), this, SLOT(rejectPairing()));
                notification->sendEvent();

            }

        } else {

            qDebug() << "Unpair request";
            if (m_pairingRequested) {
                pairingTimer.stop();
                Q_EMIT pairingFailed(i18n("Canceled by other peer"));
            }
            unpair();

        }

    } else if (!isPaired()) {

        //TODO: Alert the other side that we don't trust them
        qDebug() << "device" << name() << "not paired, ignoring package" << np.type();

    } else {

        //Forward signal
        Q_EMIT receivedPackage(np);

    }

}

void Device::acceptPairing()
{
    qDebug() << "Accepted pairing";

    KSharedConfigPtr config = KSharedConfig::openConfig("kdeconnectrc");

    //Send our own public key
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", true);
    const QByteArray& key = config->group("myself").readEntry<QByteArray>("publicKey",QByteArray());
    np.set("publicKey",key);
    bool success = sendPackage(np);

    if (!success) {
        return;
    }

    //Store as trusted device
    m_publicKey = m_tempPublicKey;
    config->group("devices").group(id()).writeEntry("publicKey", m_tempPublicKey.toPem());
    config->group("devices").group(id()).writeEntry("name", name());

    reloadPlugins(); //This will load plugins
}

void Device::rejectPairing()
{
    qDebug() << "Rejected pairing";

    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", false);
    sendPackage(np);

    KNotification* notification = (KNotification*)sender();
    notification->setActions(QStringList());
    notification->setText(i18n("Pairing rejected"));
    notification->update();

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

