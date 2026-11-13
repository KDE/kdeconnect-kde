/**
 * SPDX-FileCopyrightText: 2026 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QWindow>

#if defined(Q_OS_MACOS)
#include "macosforegroundapp.h"
#else
#include <KWindowSystem>
#endif

inline void raiseWindow(QWindow *window)
{
    window->setWindowStates(window->windowStates() & ~Qt::WindowMinimized);
#if defined(Q_OS_MACOS)
    activateWindowForMacOS(window);
#else
    KWindowSystem::updateStartupId(window);
    KWindowSystem::activateWindow(window);
#endif
}
