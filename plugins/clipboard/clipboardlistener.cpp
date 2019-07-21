/**
 * Copyright 2016 Albert Vaca <albertvaka@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "clipboardlistener.h"
#include "clipboardlistenerqt.h"
#ifdef WITH_KWAYLAND
#include "clipboardlistenerwayland.h"
#endif

ClipboardListener::ClipboardListener()
{}

ClipboardListener* ClipboardListener::instance()
{
    static ClipboardListener* me = nullptr;
    if (!me) {
#ifdef WITH_KWAYLAND
        if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive)) {
            me = new ClipboardListenerWayland();
        }
        else
#endif
        {
            me = new ClipboardListenerQt();
        }
    }
    return me;
}
