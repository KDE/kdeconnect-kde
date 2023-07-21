/**
 * SPDX-FileCopyrightText: 2019 Weixuan XIAO <veyx.shaw@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>
#include <QProcess>

#include <core/kdeconnectplugin.h>

class Q_DECL_EXPORT ScreensaverInhibitPlugin : public KdeConnectPlugin
{
    Q_OBJECT

public:
    explicit ScreensaverInhibitPlugin(QObject *parent, const QVariantList &args);
    ~ScreensaverInhibitPlugin() override;

    bool receivePacket(const NetworkPacket &np) override;

private:
    QProcess *m_caffeinateProcess;
};
