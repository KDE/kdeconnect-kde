/**
 * SPDX-FileCopyrightText: 2023 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef PAIR_STATE_H
#define PAIR_STATE_H

enum class PairState {
    NotPaired,
    Requested,
    RequestedByPeer,
    Paired,
};

#endif
