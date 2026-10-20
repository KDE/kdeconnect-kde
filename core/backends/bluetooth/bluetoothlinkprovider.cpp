/**
 * SPDX-FileCopyrightText: 2025 Rob Emery <git@mintsoft.net>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "bluetoothlinkprovider.h"

#include "bluetoothlinkproviderimpl.h"

BluetoothLinkProvider::BluetoothLinkProvider(bool isDisabled)
    : wrappedInstance(new BluetoothLinkProviderImpl(isDisabled, this))
    , connectTimer(new QTimer(this))
{
    connect(wrappedInstance, &BluetoothLinkProviderImpl::connectionReceived, this, &BluetoothLinkProvider::onNewDeviceLink);

    connectTimer->setInterval(30000);
    connect(connectTimer, &QTimer::timeout, wrappedInstance, &BluetoothLinkProviderImpl::onStartDiscovery);
}

BluetoothLinkProvider::~BluetoothLinkProvider()
{
}

void BluetoothLinkProvider::enable()
{
    wrappedInstance->enable();
    connectTimer->start();
}

void BluetoothLinkProvider::disable()
{
    wrappedInstance->disable();
    connectTimer->stop();
}

void BluetoothLinkProvider::onStart()
{
    wrappedInstance->onStart();
    connectTimer->start();
}

void BluetoothLinkProvider::onStop()
{
    wrappedInstance->onStop();
    connectTimer->stop();
}

void BluetoothLinkProvider::onNetworkChange()
{
    wrappedInstance->onNetworkChange();
}

void BluetoothLinkProvider::onLinkDestroyed(const QString &a, DeviceLink *b)
{
    wrappedInstance->onLinkDestroyed(a, b);
}

void BluetoothLinkProvider::onNewDeviceLink(DeviceLink *link)
{
    Q_EMIT LinkProvider::connectionReceived(link);
}
