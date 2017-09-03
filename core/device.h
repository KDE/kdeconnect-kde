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
#include <QVector>
#include <QSet>
#include <QHostAddress>

#include "networkpackage.h"
#include "backends/devicelink.h"

class DeviceLink;
class KdeConnectPlugin;

class KDECONNECTCORE_EXPORT Device
    : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device")
    Q_PROPERTY(QString type READ type CONSTANT)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString iconName READ iconName CONSTANT)
    Q_PROPERTY(QString statusIconName READ statusIconName)
    Q_PROPERTY(bool isReachable READ isReachable NOTIFY reachableChanged)
    Q_PROPERTY(bool isTrusted READ isTrusted NOTIFY trustedChanged)
    Q_PROPERTY(QStringList supportedPlugins READ supportedPlugins NOTIFY pluginsChanged)
    Q_PROPERTY(bool hasPairingRequests READ hasPairingRequests NOTIFY hasPairingRequestsChanged)

public:

    enum DeviceType {
        Unknown,
        Desktop,
        Laptop,
        Phone,
        Tablet,
    };

    /**
     * Restores the @p device from the saved configuration
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

    ~Device() override;

    QString id() const { return m_deviceId; }
    QString name() const { return m_deviceName; }
    QString dbusPath() const { return "/modules/kdeconnect/devices/"+id(); }
    QString type() const { return type2str(m_deviceType); }
    QString iconName() const;
    QString statusIconName() const;
    Q_SCRIPTABLE QString encryptionInfo() const;

    //Add and remove links
    void addLink(const NetworkPackage& identityPackage, DeviceLink*);
    void removeLink(DeviceLink*);

    Q_SCRIPTABLE bool isTrusted() const;

    Q_SCRIPTABLE QStringList availableLinks() const;
    bool isReachable() const { return !m_deviceLinks.isEmpty(); }

    Q_SCRIPTABLE QStringList loadedPlugins() const;
    Q_SCRIPTABLE bool hasPlugin(const QString& name) const;

    Q_SCRIPTABLE QString pluginsConfigFile() const;

    KdeConnectPlugin* plugin(const QString& pluginName) const;
    Q_SCRIPTABLE void setPluginEnabled(const QString& pluginName, bool enabled);
    Q_SCRIPTABLE bool isPluginEnabled(const QString& pluginName) const;

    void cleanUnneededLinks();

    int protocolVersion() { return m_protocolVersion; }
    QStringList supportedPlugins() const { return m_supportedPlugins.toList(); }

    QHostAddress getLocalIpAddress() const;

public Q_SLOTS:
    ///sends a @p np package to the device
    ///virtual for testing purposes.
    virtual bool sendPackage(NetworkPackage& np);

    //Dbus operations
public Q_SLOTS:
    Q_SCRIPTABLE void requestPair(); //to all links
    Q_SCRIPTABLE void unpair(); //from all links
    Q_SCRIPTABLE void reloadPlugins(); //from kconf

    Q_SCRIPTABLE void acceptPairing();
    Q_SCRIPTABLE void rejectPairing();
    Q_SCRIPTABLE bool hasPairingRequests() const;

private Q_SLOTS:
    void privateReceivedPackage(const NetworkPackage& np);
    void linkDestroyed(QObject* o);
    void pairStatusChanged(DeviceLink::PairStatus current);
    void addPairingRequest(PairingHandler* handler);
    void removePairingRequest(PairingHandler* handler);

Q_SIGNALS:
    Q_SCRIPTABLE void pluginsChanged();
    Q_SCRIPTABLE void reachableChanged(bool reachable);
    Q_SCRIPTABLE void trustedChanged(bool trusted);
    Q_SCRIPTABLE void pairingError(const QString& error);
    Q_SCRIPTABLE void nameChanged(const QString& name);

    Q_SCRIPTABLE void hasPairingRequestsChanged(bool hasPairingRequests);

private: //Methods
    static DeviceType str2type(const QString& deviceType);
    static QString type2str(DeviceType deviceType);

    void setName(const QString& name);
    QString iconForStatus(bool reachable, bool paired) const;

private: //Fields (TODO: dPointer!)
    const QString m_deviceId;
    QString m_deviceName;
    DeviceType m_deviceType;
    int m_protocolVersion;

    QVector<DeviceLink*> m_deviceLinks;
    QHash<QString, KdeConnectPlugin*> m_plugins;

    //Capabilities stuff
    QMultiMap<QString, KdeConnectPlugin*> m_pluginsByIncomingCapability;
    QSet<QString> m_supportedPlugins;
    QSet<PairingHandler*> m_pairRequests;
};

Q_DECLARE_METATYPE(Device*)

#endif // DEVICE_H
