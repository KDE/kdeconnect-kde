/**
 * SPDX-FileCopyrightText: 2025 Rob Emery <git@mintsoft.net>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "bluetoothlinkprovider.h"

#ifdef Q_OS_LINUX
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusServiceWatcher>
#endif

#include "bluetoothlinkproviderimpl.h"
#include "core_debug.h"

#ifdef Q_OS_LINUX
static const QString BLUEZ_SERVICE = QStringLiteral("org.bluez");
#endif

BluetoothLinkProvider::BluetoothLinkProvider(bool isDisabled)
    : wrappedInstance(nullptr)
    , connectTimer(new QTimer(this))
    , started(false)
    , enabled(!isDisabled)
#ifdef Q_OS_LINUX
    , bluezRunning(false)
#endif
{
    connectTimer->setInterval(30000);

#ifdef Q_OS_LINUX
    // Qt Bluetooth makes blocking DBus calls to BlueZ, which gets DBus-activated if it's not running.
    // If it takes long to start, we could block here for 25 seconds (dbus default timeout).
    // Instead, wait for it to start asynchronously.
    auto bluezWatcher = new QDBusServiceWatcher(BLUEZ_SERVICE,
                                                QDBusConnection::systemBus(),
                                                QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration,
                                                this);
    connect(bluezWatcher, &QDBusServiceWatcher::serviceRegistered, this, [this]() {
        qCDebug(KDECONNECT_CORE) << "BlueZ started";
        bluezRunning = true;
        updateImpl();
    });
    connect(bluezWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this]() {
        qCDebug(KDECONNECT_CORE) << "BlueZ stopped";
        bluezRunning = false;
        updateImpl();
    });
    // Query the initial state
    if (QDBusConnectionInterface *bus = QDBusConnection::systemBus().interface()) {
        bluezRunning = bus->isServiceRegistered(BLUEZ_SERVICE);
    }
#endif
}

BluetoothLinkProvider::~BluetoothLinkProvider()
{
}

void BluetoothLinkProvider::enable()
{
    enabled = true;
#ifdef Q_OS_LINUX
    startBluezAsync();
#endif
    updateImpl();
}

void BluetoothLinkProvider::disable()
{
    enabled = false;
    updateImpl();
}

void BluetoothLinkProvider::onStart()
{
    started = true;
#ifdef Q_OS_LINUX
    startBluezAsync();
#endif
    updateImpl();
}

void BluetoothLinkProvider::onStop()
{
    started = false;
    updateImpl();
}

void BluetoothLinkProvider::onNetworkChange()
{
    if (wrappedInstance) {
        wrappedInstance->onNetworkChange();
    }
}

void BluetoothLinkProvider::onLinkDestroyed(const QString &a, DeviceLink *b)
{
    if (wrappedInstance) {
        wrappedInstance->onLinkDestroyed(a, b);
    }
}

void BluetoothLinkProvider::onNewDeviceLink(DeviceLink *link)
{
    Q_EMIT LinkProvider::connectionReceived(link);
}

#ifdef Q_OS_LINUX
void BluetoothLinkProvider::startBluezAsync()
{
    if (!started || !enabled || bluezRunning) {
        return;
    }
    QDBusConnectionInterface *bus = QDBusConnection::systemBus().interface();
    if (!bus) {
        return;
    }
    // bluezWatcher will update bluezRunning if it starts
    QDBusPendingCall call = bus->asyncCall(QStringLiteral("StartServiceByName"), BLUEZ_SERVICE, 0u);
    auto watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [watcher]() {
        watcher->deleteLater();
        if (watcher->isError()) {
            qCWarning(KDECONNECT_CORE) << "Could not start BlueZ:" << watcher->error().message();
        }
    });
}
#endif

void BluetoothLinkProvider::updateImpl()
{
    // Creates or destroys wrappedInstance, which should exist only when started, enabled and BlueZ is running
#ifdef Q_OS_LINUX
    const bool shouldRun = started && enabled && bluezRunning;
#else
    const bool shouldRun = started && enabled;
#endif

    if (shouldRun && !wrappedInstance) {
        qCDebug(KDECONNECT_CORE) << "Starting the Bluetooth link provider";
        wrappedInstance = new BluetoothLinkProviderImpl(false, this);
        connect(wrappedInstance, &BluetoothLinkProviderImpl::connectionReceived, this, &BluetoothLinkProvider::onNewDeviceLink);
        connect(connectTimer, &QTimer::timeout, wrappedInstance, &BluetoothLinkProviderImpl::onStartDiscovery);
        wrappedInstance->onStart();
        connectTimer->start();
    } else if (!shouldRun && wrappedInstance) {
        qCDebug(KDECONNECT_CORE) << "Stopping the Bluetooth link provider";
        connectTimer->stop();
#ifdef Q_OS_LINUX
        // Unregister our service cleanly, unless BlueZ is gone and the call could block
        if (bluezRunning)
#endif
        {
            wrappedInstance->onStop();
        }
        delete wrappedInstance;
        wrappedInstance = nullptr;
    }
}
