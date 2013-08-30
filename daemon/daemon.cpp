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

#include "networkpackage.h"

#include "linkproviders/lanlinkprovider.h"
#include "linkproviders/loopbacklinkprovider.h"

#include <QUuid>
#include <QDBusConnection>
#include <QNetworkSession>
#include <QNetworkConfigurationManager>
#include <QSslKey>

#include <KIcon>
#include <KConfigGroup>

K_PLUGIN_FACTORY(KdeConnectFactory, registerPlugin<Daemon>();)
K_EXPORT_PLUGIN(KdeConnectFactory("kdeconnect", "kdeconnect"))

Daemon::Daemon(QObject *parent, const QList<QVariant>&)
    : KDEDModule(parent)
{

    KSharedConfigPtr config = KSharedConfig::openConfig("kdeconnectrc");

    if (!config->group("myself").hasKey("id")) {
        QString uuid = QUuid::createUuid().toString();
        //uuids contain charcaters that are not exportable in dbus paths
        uuid = uuid.mid(1, uuid.length() - 2).replace("-", "_");
        config->group("myself").writeEntry("id", uuid);
        qDebug() << "My id:" << uuid;
    }

    if (!config->group("myself").hasKey("privateKey")) {

        //TODO: Generate

        QByteArray key = QByteArray(
                                "-----BEGIN RSA PRIVATE KEY-----\n"
                                "MIICXAIBAAKBgQCnKxy6aZrABVvbxuWqMPbohH4KRDBGqyO/OwxvUD1qHpqZ9cJT\n"
                                "bgttiIaXzdQny5esf6brI6Di/ssIp9awdLBlMT+eR6zR7g446tbxaCFuUiL0QIei\n"
                                "izEveTDNRbson/8DPJrn8/81doTeXsuV7YbqmtUGwdZ5kiocAW92ZZukdQIDAQAB\n"
                                "AoGBAI18yuLoMQdnQblBne8vZDumsDsmPaoCfc4EP2ETi/d+kaHPxTryABAkJq7j\n"
                                "kjZgdi6VGIUacbjOqK/Zxrcw/H460EwOUzh97Z4t9CDtDhz6t3ddT8CfbG2TUgbx\n"
                                "Vv3mSYSUDBdNBV6YY4fyLtZl6oI2V+rBaFIT48+vAK9doKlhAkEA2ZKm9dc80IjU\n"
                                "c/Wwn8ij+6ALs4Mpa0dPYivgZ2QhXiX5TfMymal2dDufkOH4wIUO+8vV8CSmmTRU\n"
                                "8Lv/B3pY7QJBAMSxeJtTSFwBcGRaZKRMIqeuZ/yMMT4EqqIh1DjBpujCRKApVpkO\n"
                                "kVx3Yu7xyOfniXBwujiYNSL6LrWdKykEsKkCQEr2UDgbtIRU4H4jhHtI8dbcSavL\n"
                                "4RVpOFymqWZ2BVke1EqbJC/1Ry687DlK4h3Sulre3BMlTZEziqB25WN6L/ECQBJv\n"
                                "B3yXG4rz35KoHhJ/yCeq4rf6c4r6aPt07Cy9iWT6/+96sFD72oet8KmwI0IIowrU\n"
                                "pb80FJbIl6QRrL/VXrECQBDdeCAG6J3Cwm4ozQiDQyiNd1qJqWc4co9savJxLtEU\n"
                                "s5L4Qwfrexm16oCJimGmsa5q6Y0n4f5gY+MRh3n+nQo=\n"
                                "-----END RSA PRIVATE KEY-----\n"
                             );

        //Test for validity
        //QSslKey privateKey(key.toAscii(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
        //qDebug() << "Valid private key:" << !privateKey.isNull();

        config->group("myself").writeEntry("privateKey", key);

    }

    if (!config->group("myself").hasKey("publicKey")) {

        //TODO: Generate

        QByteArray key = QByteArray(
                                "-----BEGIN PUBLIC KEY-----\n"
                                "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCnKxy6aZrABVvbxuWqMPbohH4K\n"
                                "RDBGqyO/OwxvUD1qHpqZ9cJTbgttiIaXzdQny5esf6brI6Di/ssIp9awdLBlMT+e\n"
                                "R6zR7g446tbxaCFuUiL0QIeiizEveTDNRbson/8DPJrn8/81doTeXsuV7YbqmtUG\n"
                                "wdZ5kiocAW92ZZukdQIDAQAB\n"
                                "-----END PUBLIC KEY-----\n"

                             );

        //Test for validity
        //QSslKey publicKey(key.toAscii(), QSsl::Rsa, QSsl::Pem, QSsl::PublicKey);
        //qDebug() << "Valid public key:" << !publicKey.isNull();

        config->group("myself").writeEntry("publicKey", key);

    }

    //Debugging
    qDebug() << "Starting KdeConnect daemon";

    //Load backends (hardcoded by now, should be plugins in a future)
    mLinkProviders.insert(new LanLinkProvider());
    mLinkProviders.insert(new LoopbackLinkProvider());

    //Read remebered paired devices
    const KConfigGroup& known = config->group("devices");
    const QStringList& list = known.groupList();
    Q_FOREACH(const QString& id, list) {
        Device* device = new Device(id);
        connect(device, SIGNAL(reachableStatusChanged()), this, SLOT(onDeviceReachableStatusChanged()));
        mDevices[id] = device;
        Q_EMIT deviceAdded(id);
    }
    
    QNetworkSession* network = new QNetworkSession(QNetworkConfigurationManager().defaultConfiguration());

    //Listen to incomming connections
    Q_FOREACH (LinkProvider* a, mLinkProviders) {
        connect(network, SIGNAL(stateChanged(QNetworkSession::State)),
                a, SLOT(onNetworkChange(QNetworkSession::State)));
        connect(a, SIGNAL(onConnectionReceived(NetworkPackage, DeviceLink*)),
                this, SLOT(onNewDeviceLink(NetworkPackage, DeviceLink*)));
    }

    QDBusConnection::sessionBus().registerService("org.kde.kdeconnect");

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

QStringList Daemon::visibleDevices()
{
    QStringList ret;
    Q_FOREACH(Device* device, mDevices) {
        if (device->reachable()) {
            ret.append(device->id());
        }
    }
    return ret;
}

QStringList Daemon::devices()
{
    return mDevices.keys();
}

void Daemon::onNewDeviceLink(const NetworkPackage& identityPackage, DeviceLink* dl)
{
    const QString& id = identityPackage.get<QString>("deviceId");

    qDebug() << "Device discovered" << id << "via" << dl->provider()->name();

    if (mDevices.contains(id)) {
        qDebug() << "It is a known device";
        Device* device = mDevices[id];
        device->addLink(dl);
    } else {
        qDebug() << "It is a new device";

        Device* device = new Device(identityPackage, dl);
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

    Q_EMIT deviceVisibilityChanged(id, device->reachable());

    if (!device->reachable()) {

        if (!device->isPaired()) {
            qDebug() << "Destroying device";
            Q_EMIT deviceRemoved(id);
            mDevices.remove(id);
            device->deleteLater();
        }

    }

}

Daemon::~Daemon()
{

}

