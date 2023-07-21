/**
 * SPDX-FileCopyrightText: 2019 Weixuan XIAO <veyx.shaw@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "abstractremoteinput.h"

class MacOSRemoteInput : public AbstractRemoteInput
{
    Q_OBJECT

public:
    explicit MacOSRemoteInput(QObject *parent);

    bool handlePacket(const NetworkPacket &np) override;
    bool hasKeyboardSupport() override;
};
