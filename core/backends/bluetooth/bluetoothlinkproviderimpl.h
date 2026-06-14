/**
 * SPDX-FileCopyrightText: 2016 Saikrishna Arcot <saiarcot895@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QBluetoothAddress>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothLocalDevice>
#include <QBluetoothServer>
#include <QBluetoothServiceDiscoveryAgent>
#include <QBluetoothSocket>
#include <QBluetoothUuid>
#include <QObject>
#include <QPointer>

#include "../devicelink.h"

class BluetoothDeviceLink;
class ConnectionMultiplexer;
class MultiplexChannel;
class BluetoothLinkProvider;

class BluetoothLinkProviderImpl : public QObject
{
    Q_OBJECT

public:
    BluetoothLinkProviderImpl(bool disabled = false, BluetoothLinkProvider *parent = nullptr);

    ~BluetoothLinkProviderImpl() override;

    void enable();
    void disable();

public Q_SLOTS:
    virtual void onNetworkChange();
    virtual void onStart();
    virtual void onStop();
    virtual void onLinkDestroyed(const QString &deviceId, DeviceLink *oldPtr);
    void onStartDiscovery();
    void connectError();

private Q_SLOTS:
    void socketDisconnected(const QBluetoothAddress &peerAddress, MultiplexChannel *socket);

    void serverNewConnection();
    void serverDataReceived(const QBluetoothAddress &peerAddress, QSharedPointer<MultiplexChannel> socket);
    void clientConnected(QPointer<QBluetoothSocket> socket);
    void clientIdentityReceived(const QBluetoothAddress &peerAddress, QSharedPointer<MultiplexChannel> socket);

    void serviceDiscovered(const QBluetoothServiceInfo &info);

Q_SIGNALS:
    void onConnectionReceived(DeviceLink *);

private:
    void addLink(BluetoothDeviceLink *deviceLink, const QString &deviceId);
    QList<QBluetoothAddress> getPairedDevices();
    void tryToInitialise();

    QBluetoothUuid mServiceUuid;
    QPointer<QBluetoothServer> mBluetoothServer;
    QBluetoothServiceInfo mKdeconnectService;
    QBluetoothServiceDiscoveryAgent *mServiceDiscoveryAgent;
    bool mDisabled;

    QMap<QString, DeviceLink *> mLinks;

    QMap<QBluetoothAddress, ConnectionMultiplexer *> mSockets;
    BluetoothLinkProvider *mParent;
};
