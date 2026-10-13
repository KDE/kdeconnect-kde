/**
 * SPDX-FileCopyrightText: 2026 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <core/kdeconnectplugin.h>

#define PACKET_TYPE_MPRIS QStringLiteral("kdeconnect.mpris")

class MprisControlPlugin : public KdeConnectPlugin
{
    Q_OBJECT

public:
    explicit MprisControlPlugin(QObject *parent, const QVariantList &args);

    void receivePacket(const NetworkPacket &np) override;

private:
    void sendPlayerList();
};
