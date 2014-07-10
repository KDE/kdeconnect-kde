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

#ifndef DEVICE_H
#define DEVICE_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QSslKey>
#include <QTimer>
#include <QtCrypto>

#include "networkpackage.h"

class DeviceLink;
class KdeConnectPlugin;

class KDECONNECTCORE_EXPORT Device
    : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device")
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString iconName READ iconName CONSTANT)
    Q_PROPERTY(QString name READ name)

    enum PairStatus {
        NotPaired,
        Requested,
        RequestedByPeer,
        Paired,
    };

    enum DeviceType {
        Unknown,
        Desktop,
        Laptop,
        Phone,
        Tablet,
    };
    static DeviceType str2type(QString deviceType);
    static QString type2str(DeviceType deviceType);

public:
    /**
     * Reads the @p device from KConfig
     *
     * We already know it but we need to wait for an incoming DeviceLink to communicate
     */
    Device(QObject* parent, const QString& id);

    /**
     * Device known via an incoming connection sent to us via a devicelink.
     *
     * We know everything but we don't trust it yet
     */
    Device(QObject* parent, const NetworkPackage& np, DeviceLink* dl);

    virtual ~Device();

    QString id() const { return m_deviceId; }
    QString name() const { return m_deviceName; }
    QString dbusPath() const { return "/modules/kdeconnect/devices/"+id(); }
    QString iconName() const;

    //Add and remove links
    void addLink(const NetworkPackage& identityPackage, DeviceLink*);
    void removeLink(DeviceLink*);

    QString privateKeyPath() const;
    
    Q_SCRIPTABLE bool isPaired() const { return m_pairStatus==Device::Paired; }
    Q_SCRIPTABLE bool pairRequested() const { return m_pairStatus==Device::Requested; }

    Q_SCRIPTABLE QStringList availableLinks() const;
    Q_SCRIPTABLE bool isReachable() const { return !m_deviceLinks.isEmpty(); }

    Q_SCRIPTABLE QStringList loadedPlugins() const;
    Q_SCRIPTABLE bool hasPlugin(const QString& name) const;

public Q_SLOTS:
    ///sends a @p np package to the device
    virtual bool sendPackage(NetworkPackage& np);

    //Dbus operations
public Q_SLOTS:
    Q_SCRIPTABLE void requestPair();
    Q_SCRIPTABLE void unpair();
    Q_SCRIPTABLE void reloadPlugins(); //From kconf
    void acceptPairing();
    void rejectPairing();

private Q_SLOTS:
    void privateReceivedPackage(const NetworkPackage& np);
    void linkDestroyed(QObject* o);
    void pairingTimeout();

Q_SIGNALS:
    Q_SCRIPTABLE void reachableStatusChanged();
    Q_SCRIPTABLE void pluginsChanged();
    Q_SCRIPTABLE void pairingSuccesful();
    Q_SCRIPTABLE void pairingFailed(const QString& error);
    Q_SCRIPTABLE void unpaired();

private:
    const QString m_deviceId;
    QString m_deviceName;
    DeviceType m_deviceType;
    QCA::PrivateKey m_privateKey;
    QCA::PublicKey m_publicKey;
    PairStatus m_pairStatus;
    int m_protocolVersion;

    QList<DeviceLink*> m_deviceLinks;
    QMap<QString, KdeConnectPlugin*> m_plugins;
    QMultiMap<QString, KdeConnectPlugin*> m_pluginsByIncomingInterface;
    QMultiMap<QString, KdeConnectPlugin*> m_pluginsByOutgoingInterface;

    QTimer m_pairingTimeut;
    QSet<QString> m_incomingCapabilities;
    QSet<QString> m_outgoingCapabilities;

    void setAsPaired();
    void storeAsTrusted();
    bool sendOwnPublicKey();
    void initPrivateKey();

};

Q_DECLARE_METATYPE(Device*)

#endif // DEVICE_H
