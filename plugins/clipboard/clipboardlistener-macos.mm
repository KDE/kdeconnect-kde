/**
 * SPDX-FileCopyrightText: 2026 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "clipboardlistener.h"

#import <AppKit/AppKit.h>

long ClipboardListener::macosPasteboardChangeCount()
{
    return [NSPasteboard generalPasteboard].changeCount;
}
