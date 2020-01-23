/**
 * SPDX-FileCopyrightText: 2018 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "abstractremoteinput.h"

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_MOUSEPAD, "kdeconnect.plugin.mousepad")
AbstractRemoteInput::AbstractRemoteInput(QObject *parent)
    : QObject(parent)
{
}
