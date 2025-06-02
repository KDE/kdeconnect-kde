/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef DEVICE_H
#define DEVICE_H

#include <QHostAddress>
#include <QObject>
#include <QString>

#include "backends/devicelink.h"
#include "deviceinfo.h"
#include "networkpacket.h"
#include "pairstate.h"

class DeviceLink;
class KdeConnectPlugin;

class KDECONNECTCORE_EXPORT Device : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device")
    Q_PROPERTY(QString type READ typeAsString NOTIFY typeChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString iconName READ iconName CONSTANT)
    Q_PROPERTY(QString verificationKey READ verificationKey NOTIFY pairStateChanged)
    Q_PROPERTY(QString statusIconName READ statusIconName NOTIFY statusIconNameChanged)
    Q_PROPERTY(bool isReachable READ isReachable NOTIFY reachableChanged)
    Q_PROPERTY(bool isPaired READ isPaired NOTIFY pairStateChanged)
    Q_PROPERTY(bool isPairRequested READ isPairRequested NOTIFY pairStateChanged)
    Q_PROPERTY(bool isPairRequestedByPeer READ isPairRequestedByPeer NOTIFY pairStateChanged)
    Q_PROPERTY(int pairState READ pairStateAsInt NOTIFY pairStateChanged)
    Q_PROPERTY(QStringList supportedPlugins READ supportedPlugins NOTIFY pluginsChanged)

public:
    /**
     * Restores the @p device from the saved configuration
     *
     * We already know it but we need to wait for an incoming DeviceLink to communicate
     */
    Device(QObject *parent, const QString &id);

    /**
     * Device known via an incoming connection sent to us via a devicelink.
     *
     * We know everything but we don't trust it yet
     */
    Device(QObject *parent, DeviceLink *link);

    ~Device() override;

    QString id() const;
    QString name() const;
    QString dbusPath() const
    {
        return QStringLiteral("/modules/kdeconnect/devices/") + id();
    }
    DeviceType type() const;
    QString typeAsString() const
    {
        return type().toString();
    };
    QString iconName() const;
    QString statusIconName() const;
    Q_SCRIPTABLE QString verificationKey() const;
    Q_SCRIPTABLE QString encryptionInfo() const;

    // Add and remove links
    void addLink(DeviceLink *link);
    void removeLink(DeviceLink *link);

    bool updateDeviceInfo(const DeviceInfo &deviceInfo);

    bool hasInvalidCertificate();
    QSslCertificate certificate() const;

    PairState pairState() const;
    Q_SCRIPTABLE int pairStateAsInt() const; // Hack because qdbus doesn't like enums
    Q_SCRIPTABLE bool isPaired() const;
    Q_SCRIPTABLE bool isPairRequested() const;
    Q_SCRIPTABLE bool isPairRequestedByPeer() const;
    virtual bool isReachable() const;

    Q_SCRIPTABLE QStringList loadedPlugins() const;
    Q_SCRIPTABLE bool hasPlugin(const QString &name) const;

    Q_SCRIPTABLE QString pluginsConfigFile() const;

    KdeConnectPlugin *plugin(const QString &pluginName) const;
    Q_SCRIPTABLE void setPluginEnabled(const QString &pluginName, bool enabled);
    Q_SCRIPTABLE bool isPluginEnabled(const QString &pluginName) const;

    int protocolVersion();
    QStringList supportedPlugins() const;

    QHostAddress getLocalIpAddress() const;

public Q_SLOTS:
    /// sends a @p np packet to the device
    /// virtual for testing purposes.
    virtual bool sendPacket(NetworkPacket &np);

    // Dbus operations
public Q_SLOTS:
    Q_SCRIPTABLE void requestPairing();
    Q_SCRIPTABLE void unpair();
    Q_SCRIPTABLE void reloadPlugins(); // from kconf

    Q_SCRIPTABLE void acceptPairing();
    Q_SCRIPTABLE void cancelPairing();

    Q_SCRIPTABLE QString pluginIconName(const QString &pluginName);
private Q_SLOTS:
    void privateReceivedPacket(const NetworkPacket &np);
    void linkDestroyed(QObject *o);

    void pairingHandler_incomingPairRequest();
    void pairingHandler_pairingFailed(const QString &errorMessage);
    void pairingHandler_pairingSuccessful();
    void pairingHandler_unpaired();

Q_SIGNALS:
    Q_SCRIPTABLE void pluginsChanged();
    Q_SCRIPTABLE void reachableChanged(bool reachable);
    Q_SCRIPTABLE void pairStateChanged(int pairState); // Hack because qdbus doesn't like enums
    Q_SCRIPTABLE void pairingFailed(const QString &error);
    Q_SCRIPTABLE void nameChanged(const QString &name);
    Q_SCRIPTABLE void typeChanged(const QString &type);
    Q_SCRIPTABLE void statusIconNameChanged();

private:
    class DevicePrivate;
    DevicePrivate *d;
};

Q_DECLARE_METATYPE(Device *)

#endif // DEVICE_H
