/**
 * SPDX-FileCopyrightText: 2025 Martin Sh <hemisputnik@proton.me>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

// This plugin allows users to use their devices as pressure sensitive drawing
// tablets.

#pragma once

#include "abstractdigitizerimpl.h"

#include <QObject>

#include <core/kdeconnectplugin.h>

#define PACKET_TYPE_DIGITIZER_SESSION QStringLiteral("kdeconnect.digitizer.session")
#define PACKET_TYPE_DIGITIZER QStringLiteral("kdeconnect.digitizer")

class DigitizerPlugin : public KdeConnectPlugin
{
    Q_OBJECT

public:
    explicit DigitizerPlugin(QObject *parent, const QVariantList &args);
    ~DigitizerPlugin() override;

    void receivePacket(const NetworkPacket &np) override;

private:
    AbstractDigitizerImpl *m_impl;
};
