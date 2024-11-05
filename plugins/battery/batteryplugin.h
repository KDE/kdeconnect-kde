/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <core/kdeconnectplugin.h>

#define PACKET_TYPE_BATTERY QStringLiteral("kdeconnect.battery")

class BatteryPlugin : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.battery")
    Q_PROPERTY(int charge READ charge NOTIFY refreshed)
    Q_PROPERTY(bool isCharging READ isCharging NOTIFY refreshed)

public:
    explicit BatteryPlugin(QObject *parent, const QVariantList &args);

    void receivePacket(const NetworkPacket &np) override;
    void connected() override;
    QString dbusPath() const override;

    int charge() const;
    bool isCharging() const;

Q_SIGNALS:
    Q_SCRIPTABLE void refreshed(bool isCharging, int charge);

private:
    void slotChargeChanged();

    // Keep these values in sync with THRESHOLD* constants in
    // kdeconnect-android:BatteryPlugin.java
    // see README for their meaning
    enum ThresholdBatteryEvent {
        ThresholdNone = 0,
        ThresholdBatteryLow = 1,
    };

    int m_charge = -1;
    bool m_isCharging = false;
};
