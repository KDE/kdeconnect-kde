/**
 * SPDX-FileCopyrightText: 2025 Rob Emery <git@mintsoft.net>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "asynclinkprovider.h"
#include "bluetoothlinkprovider.h"
#include <qthread.h>
#include <qtmetamacros.h>

AsyncLinkProvider::AsyncLinkProvider()
{
}

AsyncLinkProvider::~AsyncLinkProvider()
{
}

AsyncLinkProvider::AsyncLinkProvider(bool isDisabled)
{
    startDisabled = isDisabled;

    this->moveToThread(&workerThread);

    connect(&workerThread, &QThread::started, this, [this, isDisabled]() {
        wrappedInstance = new BluetoothLinkProvider(isDisabled);

        connect(wrappedInstance, &LinkProvider::onConnectionReceived, this, &AsyncLinkProvider::onNewDeviceLink);

        connect(this, &AsyncLinkProvider::enableRequestEvent, wrappedInstance, &BluetoothLinkProvider::enable);
        connect(this, &AsyncLinkProvider::disableRequestEvent, wrappedInstance, &BluetoothLinkProvider::disable);
        connect(this, &AsyncLinkProvider::startRequestEvent, wrappedInstance, &BluetoothLinkProvider::onStart);
        connect(this, &AsyncLinkProvider::stopRequestEvent, wrappedInstance, &BluetoothLinkProvider::onStop);
        connect(this, &AsyncLinkProvider::networkChangeEvent, wrappedInstance, &BluetoothLinkProvider::onNetworkChange);

        connectTimer = new QTimer();
        connectTimer->setInterval(30000);
        connect(connectTimer, &QTimer::timeout, wrappedInstance, &BluetoothLinkProvider::onStartDiscovery);

        connect(this, &AsyncLinkProvider::enableRequestEvent, connectTimer, qOverload<>(&QTimer::start));
        connect(this, &AsyncLinkProvider::startRequestEvent, connectTimer, qOverload<>(&QTimer::start));
        connect(this, &AsyncLinkProvider::disableRequestEvent, connectTimer, &QTimer::stop);
        connect(this, &AsyncLinkProvider::stopRequestEvent, connectTimer, &QTimer::stop);
    });

    workerThread.start();
}

QString AsyncLinkProvider::name()
{
    return QStringLiteral("AsyncLinkProvider");
}

QString AsyncLinkProvider::displayName()
{
    return i18nc("@info", "Bluetooth (beta)");
}

int AsyncLinkProvider::priority()
{
    return 10;
}

void AsyncLinkProvider::enable()
{
    Q_EMIT enableRequestEvent();
}

void AsyncLinkProvider::disable()
{
    Q_EMIT disableRequestEvent();
}

void AsyncLinkProvider::onStart()
{
    Q_EMIT startRequestEvent();
}

void AsyncLinkProvider::onStop()
{
    Q_EMIT stopRequestEvent();
}

void AsyncLinkProvider::onNetworkChange()
{
    Q_EMIT networkChangeEvent();
}

void AsyncLinkProvider::onLinkDestroyed(const QString &a, DeviceLink *b)
{
    if (wrappedInstance != nullptr) {
        wrappedInstance->onLinkDestroyed(a, b);
    }
}

void AsyncLinkProvider::onNewDeviceLink(DeviceLink *link)
{
    Q_EMIT LinkProvider::onConnectionReceived(link);
}
