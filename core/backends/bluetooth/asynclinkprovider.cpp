/**
 * SPDX-FileCopyrightText: 2025 Rob Emery <git@mintsoft.net>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "asynclinkprovider.h"
#include "bluetoothlinkprovider.h"
#include "core_debug.h"

AsyncLinkProvider::AsyncLinkProvider()
{
}

AsyncLinkProvider::~AsyncLinkProvider()
{
}

AsyncLinkProvider::AsyncLinkProvider(bool isDisabled)
    : connectTimer(new QTimer(this))
{
    startDisabled = isDisabled;
    this->moveToThread(&workerThread);
    connect(&workerThread, &QThread::started, this, &AsyncLinkProvider::runWorker);
    workerThread.start();

    eventsMutex.lock();
    events.enqueue(CONSTRUCTOR);
    eventsMutex.unlock();
}

QString AsyncLinkProvider::name()
{
    return QStringLiteral("BluetoothLinkProvider");
}

int AsyncLinkProvider::priority()
{
    return 10;
}

void AsyncLinkProvider::enable()
{
    eventsMutex.lock();
    events.enqueue(ENABLE);
    eventsMutex.unlock();
}

void AsyncLinkProvider::disable()
{
    eventsMutex.lock();
    events.enqueue(DISABLE);
    eventsMutex.unlock();
}

void AsyncLinkProvider::onStart()
{
    eventsMutex.lock();
    events.enqueue(START);
    eventsMutex.unlock();
}

void AsyncLinkProvider::onStop()
{
    eventsMutex.lock();
    events.enqueue(STOP);
    eventsMutex.unlock();
    connectTimer->stop();
}

void AsyncLinkProvider::onNetworkChange()
{
    eventsMutex.lock();
    events.enqueue(NETWORKCHANGE);
    eventsMutex.unlock();
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

void AsyncLinkProvider::runWorker()
{
    connectTimer->setInterval(30000);
    connectTimer->setSingleShot(false);

    connect(connectTimer, &QTimer::timeout, this, [this]() {
        if (wrappedInstance != nullptr) {
            wrappedInstance->onStartDiscovery();
        }
    });

    auto messagePumpTimer = new QTimer();
    messagePumpTimer->setInterval(16);
    messagePumpTimer->setSingleShot(false);

    connect(messagePumpTimer, &QTimer::timeout, this, [this]() {
        eventsMutex.lock();
        if (events.isEmpty()) {
            eventsMutex.unlock();
            return;
        }

        auto event = events.dequeue();
        eventsMutex.unlock();

        switch (event) {
        case CONSTRUCTOR:
            wrappedInstance = new BluetoothLinkProvider(startDisabled);

            connect(wrappedInstance, &LinkProvider::onConnectionReceived, this, &AsyncLinkProvider::onNewDeviceLink);
            return;
        case ENABLE:
            wrappedInstance->enable();
            break;
        case DISABLE:
            wrappedInstance->disable();
            break;
        case START:
            wrappedInstance->onStart();
            break;
        case STOP:
            wrappedInstance->onStop();
            break;
        case NETWORKCHANGE:
            wrappedInstance->onNetworkChange();
            break;
        }
    });

    connectTimer->start();
    messagePumpTimer->start();
}
