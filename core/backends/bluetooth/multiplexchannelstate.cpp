/**
 * SPDX-FileCopyrightText: 2019 Matthijs Tijink <matthijstijink@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "multiplexchannelstate.h"

MultiplexChannelState::MultiplexChannelState()
    : requestedReadAmount{0}
    , freeWriteAmount{0}
    , connected{true}
    , close_after_write{false}
{
}

#include "moc_multiplexchannelstate.cpp"
