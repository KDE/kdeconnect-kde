/**
 * Copyright 2014 Ahmed I. Khalil <albertvaka@gmail.com>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MOUSEPADPLUGIN_H
#define MOUSEPADPLUGIN_H

#include <QtGui/QCursor>
#include <core/kdeconnectplugin.h>
#include <config-mousepad.h>

struct FakeKey;

#if HAVE_WAYLAND
namespace KWayland
{
namespace Client
{
class FakeInput;
}
}
#endif

class MousepadPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT

public:
    explicit MousepadPlugin(QObject* parent, const QVariantList& args);
    ~MousepadPlugin() override;

    bool receivePackage(const NetworkPackage& np) override;
    void connected() override { }

private:
#if HAVE_X11
    bool handlePackageX11(const NetworkPackage& np);
#endif
#if HAVE_WAYLAND
    void setupWaylandIntegration();
    bool handPackageWayland(const NetworkPackage& np);
#endif

#if HAVE_X11
    FakeKey* m_fakekey;
#endif
    const bool m_x11;
#if HAVE_WAYLAND
    KWayland::Client::FakeInput* m_waylandInput;
    bool m_waylandAuthenticationRequested;
#endif
};

#endif
