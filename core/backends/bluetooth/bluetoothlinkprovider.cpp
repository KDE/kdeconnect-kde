/**
 * SPDX-FileCopyrightText: 2025 Rob Emery <git@mintsoft.net>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "bluetoothlinkprovider.h"

#include "bluetoothlinkproviderimpl.h"

BluetoothLinkProvider::BluetoothLinkProvider(bool isDisabled)
{
    startDisabled = isDisabled;

    this->moveToThread(&workerThread);

    connect(&workerThread, &QThread::started, this, [this, isDisabled]() {
        wrappedInstance = new BluetoothLinkProviderImpl(isDisabled, this);

        connect(wrappedInstance, &BluetoothLinkProviderImpl::onConnectionReceived, this, &BluetoothLinkProvider::onNewDeviceLink);

        connect(this, &BluetoothLinkProvider::enableRequestEvent, wrappedInstance, &BluetoothLinkProviderImpl::enable);
        connect(this, &BluetoothLinkProvider::disableRequestEvent, wrappedInstance, &BluetoothLinkProviderImpl::disable);
        connect(this, &BluetoothLinkProvider::startRequestEvent, wrappedInstance, &BluetoothLinkProviderImpl::onStart);
        connect(this, &BluetoothLinkProvider::stopRequestEvent, wrappedInstance, &BluetoothLinkProviderImpl::onStop);
        connect(this, &BluetoothLinkProvider::networkChangeEvent, wrappedInstance, &BluetoothLinkProviderImpl::onNetworkChange);

        connectTimer = new QTimer();
        connectTimer->setInterval(30000);
        connect(connectTimer, &QTimer::timeout, wrappedInstance, &BluetoothLinkProviderImpl::onStartDiscovery);

        connect(this, &BluetoothLinkProvider::enableRequestEvent, connectTimer, qOverload<>(&QTimer::start));
        connect(this, &BluetoothLinkProvider::startRequestEvent, connectTimer, qOverload<>(&QTimer::start));
        connect(this, &BluetoothLinkProvider::disableRequestEvent, connectTimer, &QTimer::stop);
        connect(this, &BluetoothLinkProvider::stopRequestEvent, connectTimer, &QTimer::stop);
    });

    workerThread.start();
}

BluetoothLinkProvider::~BluetoothLinkProvider()
{
}

void BluetoothLinkProvider::enable()
{
    Q_EMIT enableRequestEvent();
}

void BluetoothLinkProvider::disable()
{
    Q_EMIT disableRequestEvent();
}

void BluetoothLinkProvider::onStart()
{
    Q_EMIT startRequestEvent();
}

void BluetoothLinkProvider::onStop()
{
    Q_EMIT stopRequestEvent();
}

void BluetoothLinkProvider::onNetworkChange()
{
    Q_EMIT networkChangeEvent();
}

void BluetoothLinkProvider::onLinkDestroyed(const QString &a, DeviceLink *b)
{
    if (wrappedInstance != nullptr) {
        wrappedInstance->onLinkDestroyed(a, b);
    }
}

void BluetoothLinkProvider::onNewDeviceLink(DeviceLink *link)
{
    Q_EMIT LinkProvider::onConnectionReceived(link);
}
