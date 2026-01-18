/**
 * SPDX-FileCopyrightText: 2025 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "kdeconnectcore_export.h"

class LanLinkProvider;

class KDECONNECTCORE_EXPORT MdnsDiscovery
{
public:
    virtual ~MdnsDiscovery() {};
    virtual void onStart() = 0;
    virtual void onStop() = 0;
    virtual void onNetworkChange() = 0;
};
