/**
 * SPDX-FileCopyrightText: 2018 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "abstractremoteinput.h"

class WindowsRemoteInput : public AbstractRemoteInput
{
    Q_OBJECT

public:
    explicit WindowsRemoteInput(QObject *parent);

    bool handlePacket(const NetworkPacket &np) override;
    bool hasKeyboardSupport() override;
};
