#include "device.h"

#include <KSharedPtr>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KStandardDirs>
#include <KPluginSelector>
#include <KServiceTypeTrader>
#include <KPluginInfo>

#include <QDebug>

#include "plugins/kdeconnectplugin.h"
#include "plugins/pluginloader.h"
#include "devicelinks/devicelink.h"
#include "linkproviders/linkprovider.h"
#include "networkpackage.h"

Device::Device(const QString& id, const QString& name)
{

    m_deviceId = id;
    m_deviceName = name;
    m_paired = true;
    m_knownIdentiy = true;

    reloadPlugins();

    //Register in bus
    QDBusConnection::sessionBus().registerObject(dbusPath(), this, QDBusConnection::ExportScriptableContents | QDBusConnection::ExportAdaptors);

}

Device::Device(const QString& id, const QString& name, DeviceLink* link)
{

    m_deviceId = id;
    m_deviceName = name;
    m_paired = false;
    m_knownIdentiy = true;

    addLink(link);

    reloadPlugins();

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

    if (paired() && reachable()) { //Do not load any plugin for unpaired devices, nor useless loading them for unreachable devices

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

    Q_EMIT pluginsChanged();

}


void Device::setPair(bool b)
{
    m_paired = b;
    KSharedConfigPtr config = KSharedConfig::openConfig("kdeconnectrc");
    if (b) {
        qDebug() << name() << "paired";
        config->group("devices").group("paired").group(id()).writeEntry("name",name());
        Q_EMIT reachableStatusChanged();
    } else {
        qDebug() << name() << "unpaired";
        config->group("devices").group("paired").deleteGroup(id());
        //Do not Q_EMIT reachableStatusChanged() because we do not want it to suddenly disappear from device list
    }
    reloadPlugins();
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
        reloadPlugins();
        Q_EMIT reachableStatusChanged();
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
    if (!m_paired) {
        //qDebug() << "sendpackage disabled on untrusted device" << name();
        return false;
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
    if (np.type() == "kdeconnect.identity" && !m_knownIdentiy) {
        m_deviceName = np.get<QString>("deviceName");
    } else if (m_paired) {
        //qDebug() << "package received from trusted device" << name();
        Q_EMIT receivedPackage(np);
    } else {
        qDebug() << "device" << name() << "not trusted, ignoring package" << np.type();
    }
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

