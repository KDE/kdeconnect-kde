/**
 * SPDX-FileCopyrightText: 2026 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>

#include <memory>
#include <optional>

struct BatteryStatus {
    int charge;
    bool isCharging;
    bool isDischarging;
    int batteryQuantity;
};

// Platform-specific access to the system battery, see batterymonitor-*.cpp
class BatteryMonitor : public QObject
{
    Q_OBJECT

public:
    explicit BatteryMonitor(QObject *parent);
    ~BatteryMonitor() override;

    // Returns std::nullopt if there is no system battery
    std::optional<BatteryStatus> status() const;

Q_SIGNALS:
    void changed();

private:
    class Private;
    std::unique_ptr<Private> d;
};
