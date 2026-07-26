/**
 * SPDX-FileCopyrightText: 2026 Logan Rosen <loganrosen@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "macosforegroundapp.h"

#include <QWindow>

#import <AppKit/NSApplication.h>

void activateWindowForMacOS(QWindow *window)
{
    if (window == nullptr) {
        return;
    }

    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];
    window->show();
    window->raise();
    window->requestActivate();
}
