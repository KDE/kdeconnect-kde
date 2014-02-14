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

#include "daemon.h"

#include <QUuid>
#include <QFile>
#include <QFileInfo>
#include <QDBusConnection>
#include <QNetworkSession>
#include <QNetworkConfigurationManager>

#include <KConfig>
#include <KConfigGroup>
#include <KStandardDirs>

#include "kdebugnamespace.h"
#include "networkpackage.h"
#include "backends/lan/lanlinkprovider.h"
#include "backends/loopback/loopbacklinkprovider.h"

Daemon::Daemon(QObject *parent) : QObject(parent)
{
    KSharedConfigPtr config = KSharedConfig::openConfig("kdeconnectrc");

    if (!config->group("myself").hasKey("id")) {
        QString uuid = QUuid::createUuid().toString();
        //uuids contain charcaters that are not exportable in dbus paths
        uuid = uuid.mid(1, uuid.length() - 2).replace("-", "_");
        config->group("myself").writeEntry("id", uuid);
        kDebug(kdeconnect_kded()) << "My id:" << uuid;
    }

    //kDebug(kdeconnect_kded()) << "QCA supported capabilities:" << QCA::supportedFeatures().join(",");
    if(!QCA::isSupported("rsa")) {
        //TODO: Maybe display this in a more visible way?
        kWarning(kdeconnect_kded()) << "Error: KDE Connect could not find support for RSA in your QCA installation, if your distribution provides"
                   << "separate packages for QCA-ossl and QCA-gnupg plugins, make sure you have them installed and try again";
        return;
    }

    const QFile::Permissions strict = QFile::ReadOwner | QFile::WriteOwner | QFile::ReadUser | QFile::WriteUser;
    if (!config->group("myself").hasKey("privateKeyPath"))
    {
        const QString privateKeyPath = KStandardDirs::locateLocal("appdata", "key.pem", true, KComponentData("kdeconnect", "kdeconnect"));
        
        QFile privKey(privateKeyPath);
        
        if (!privKey.open(QIODevice::ReadWrite | QIODevice::Truncate))
        {
            kWarning(kdeconnect_kded()) << "Error: KDE Connect could not create private keys file: " << privateKeyPath;
            return;
        }
        
        if (!privKey.setPermissions(strict))
        {
            kWarning(kdeconnect_kded()) << "Error: KDE Connect could not set permissions for private file: " << privateKeyPath;
            //return;
        }

        //http://delta.affinix.com/docs/qca/rsatest_8cpp-example.html        
        privKey.write(QCA::KeyGenerator().createRSA(2048).toPEM().toAscii());
        privKey.close();
        
        config->group("myself").writeEntry("privateKeyPath", privateKeyPath);
    }
    
    if (QFile::permissions(config->group("myself").readEntry("privateKeyPath")) != strict)
    {
        kWarning(kdeconnect_kded()) << "Error: KDE Connect detects wrong permissions for private file " << config->group("myself").readEntry("privateKeyPath");
        //FIXME: Do not silently fail, because user won't notice the problem
        //return;
    }

    //Debugging
    kDebug(kdeconnect_kded()) << "Starting KdeConnect daemon";

    //Load backends (hardcoded by now, should be plugins in a future)
    mLinkProviders.insert(new LanLinkProvider());
    //mLinkProviders.insert(new LoopbackLinkProvider());

    //Read remebered paired devices
    const KConfigGroup& known = config->group("trusted_devices");
    const QStringList& list = known.groupList();
    Q_FOREACH(const QString& id, list) {
        Device* device = new Device(this, id);
        connect(device, SIGNAL(reachableStatusChanged()),
                this, SLOT(onDeviceReachableStatusChanged()));
        mDevices[id] = device;
        Q_EMIT deviceAdded(id);
    }
    
    //Listen to connectivity changes
    QNetworkSession* network = new QNetworkSession(QNetworkConfigurationManager().defaultConfiguration());
    Q_FOREACH (LinkProvider* a, mLinkProviders) {
        connect(network, SIGNAL(stateChanged(QNetworkSession::State)),
                a, SLOT(onNetworkChange(QNetworkSession::State)));
        connect(a, SIGNAL(onConnectionReceived(NetworkPackage, DeviceLink*)),
                this, SLOT(onNewDeviceLink(NetworkPackage, DeviceLink*)));
    }

    QDBusConnection::sessionBus().registerService("org.kde.kdeconnect");
    QDBusConnection::sessionBus().registerObject("/modules/kdeconnect", this, QDBusConnection::ExportScriptableContents);

    setDiscoveryEnabled(true);

}

void Daemon::setDiscoveryEnabled(bool b)
{
    //Listen to incomming connections
    Q_FOREACH (LinkProvider* a, mLinkProviders) {
        if (b)
            a->onStart();
        else
            a->onStop();
    }

}

void Daemon::forceOnNetworkChange()
{
    Q_FOREACH (LinkProvider* a, mLinkProviders) {
        a->onNetworkChange(QNetworkSession::Connected);
    }
}

QStringList Daemon::devices(bool onlyReachable, bool onlyVisible)
{
    QStringList ret;
    Q_FOREACH(Device* device, mDevices) {
        if (onlyReachable && !device->isReachable()) continue;
        if (onlyVisible && !device->isPaired()) continue;
        ret.append(device->id());
    }
    return ret;
}

void Daemon::onNewDeviceLink(const NetworkPackage& identityPackage, DeviceLink* dl)
{

    const QString& id = identityPackage.get<QString>("deviceId");

    //kDebug(kdeconnect_kded()) << "Device discovered" << id << "via" << dl->provider()->name();

    if (mDevices.contains(id)) {
        //kDebug(kdeconnect_kded()) << "It is a known device";
        Device* device = mDevices[id];
        device->addLink(identityPackage, dl);
    } else {
        //kDebug(kdeconnect_kded()) << "It is a new device";

        Device* device = new Device(this, identityPackage, dl);
        connect(device, SIGNAL(reachableStatusChanged()), this, SLOT(onDeviceReachableStatusChanged()));
        mDevices[id] = device;

        Q_EMIT deviceAdded(id);
    }

    Q_EMIT deviceVisibilityChanged(id, true);

}

void Daemon::onDeviceReachableStatusChanged()
{

    Device* device = (Device*)sender();
    QString id = device->id();

    Q_EMIT deviceVisibilityChanged(id, device->isReachable());

    //kDebug(kdeconnect_kded()) << "Device" << device->name() << "reachable status changed:" << device->isReachable();

    if (!device->isReachable()) {

        if (!device->isPaired()) {
            kDebug(kdeconnect_kded()) << "Destroying device" << device->name();
            Q_EMIT deviceRemoved(id);
            mDevices.remove(id);
            device->deleteLater();
        }

    }

}

Daemon::~Daemon()
{

}

