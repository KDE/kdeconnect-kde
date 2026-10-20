/**
 * SPDX-FileCopyrightText: 2025 Rob Emery <git@mintsoft.net>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QTimer>

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

private:
    void updateImpl();

    BluetoothLinkProviderImpl *wrappedInstance;
    QTimer *connectTimer;
    bool started;
    bool enabled;

#ifdef Q_OS_LINUX
    void startBluezAsync();
    bool bluezRunning;
#endif

private Q_SLOTS:
    void onNewDeviceLink(DeviceLink *link);
};
