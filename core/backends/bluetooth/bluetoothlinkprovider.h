/**
 * SPDX-FileCopyrightText: 2025 Rob Emery <git@mintsoft.net>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QMutex>
#include <QPointer>
#include <QQueue>
#include <QThread>
#include <QTimer>
#include <qtmetamacros.h>

#include "../devicelink.h"
#include "../linkprovider.h"

class BluetoothLinkProviderImpl;

class BluetoothLinkProvider : public LinkProvider
{
    Q_OBJECT
public:
    BluetoothLinkProvider(bool isDisabled);
    ~BluetoothLinkProvider() override;

    QString id() const override
    {
        return QStringLiteral("bluetooth");
    }

    QString displayName() const override
    {
        return i18nc("@info", "Bluetooth (beta)");
    }

    int priority() override
    {
        return 10;
    }

    void enable() override;
    void disable() override;
    void onStart() override;
    void onStop() override;
    void onNetworkChange() override;
    void onLinkDestroyed(const QString &a, DeviceLink *b) override;
Q_SIGNALS:
    void enableRequestEvent();
    void disableRequestEvent();
    void startRequestEvent();
    void stopRequestEvent();
    void networkChangeEvent();

private:
    BluetoothLinkProviderImpl *wrappedInstance;
    bool startDisabled;
    QThread workerThread;
    QTimer *connectTimer;

private Q_SLOTS:
    void onNewDeviceLink(DeviceLink *link);
};
