/**
 * SPDX-FileCopyrightText: 2018 Albert Vaca Cintora <albertvaka@gmail.com>
 * SPDX-FileCopyrightText: 2015 Martin Gräßlin <mgraesslin@kde.org>
 * SPDX-FileCopyrightText: 2014 Ahmed I. Khalil <ahmedibrahimkhali@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <config-mousepad.h>
#include <core/kdeconnectplugin.h>

#include "abstractremoteinput.h"

#define PACKET_TYPE_MOUSEPAD_KEYBOARDSTATE QLatin1String("kdeconnect.mousepad.keyboardstate")

class MousepadPlugin : public KdeConnectPlugin
{
    Q_OBJECT

public:
    explicit MousepadPlugin(QObject *parent, const QVariantList &args);
    ~MousepadPlugin() override;

    void receivePacket(const NetworkPacket &np) override;
    void connected() override;

private:
    AbstractRemoteInput *m_impl;
};
