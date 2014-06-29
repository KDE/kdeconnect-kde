/**
 * Copyright 2014 Ahmed I. Khalil <ahmedibrahimkhali@gmail.com>
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

#include "mousepadplugin.h"

#include <core/networkpackage.h>
#include <X11/extensions/XTest.h>

K_PLUGIN_FACTORY( KdeConnectPluginFactory, registerPlugin< MousepadPlugin >(); )
K_EXPORT_PLUGIN( KdeConnectPluginFactory("kdeconnect_mousepad", "kdeconnect-plugins") )

// Source: http://bharathisubramanian.wordpress.com/2010/04/01/x11-fake-mouse-events-generation-using-xtest/

MousepadPlugin::MousepadPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args), m_display(0)
{

}

MousepadPlugin::~MousepadPlugin()
{
    if (m_display) {
        XCloseDisplay(m_display);
        m_display = 0;
    }
}

bool MousepadPlugin::receivePackage(const NetworkPackage& np)
{
    float dx = np.get<float>("dx", 0);
    float dy = np.get<float>("dy", 0);

    bool isSingleClick = np.get<bool>("singleclick", false);
    bool isDoubleClick = np.get<bool>("doubleclick", false);
    bool isMiddleClick = np.get<bool>("middleclick", false);
    bool isRightClick = np.get<bool>("rightclick", false);
    bool isScroll = np.get<bool>("scroll", false);

    if (isSingleClick || isDoubleClick || isMiddleClick || isRightClick || isScroll) {
        if(!m_display) {
            m_display = XOpenDisplay(NULL);
        }

        if(m_display) {
            if (isSingleClick) {
                XTestFakeButtonEvent(m_display, LeftMouseButton, true, CurrentTime);
                XTestFakeButtonEvent(m_display, LeftMouseButton, false, CurrentTime);
            } else if (isDoubleClick) {
                XTestFakeButtonEvent(m_display, LeftMouseButton, true, CurrentTime);
                XTestFakeButtonEvent(m_display, LeftMouseButton, false, CurrentTime);
                XTestFakeButtonEvent(m_display, LeftMouseButton, true, CurrentTime);
                XTestFakeButtonEvent(m_display, LeftMouseButton, false, CurrentTime);
            } else if (isMiddleClick) {
                XTestFakeButtonEvent(m_display, MiddleMouseButton, true, CurrentTime);
                XTestFakeButtonEvent(m_display, MiddleMouseButton, false, CurrentTime);
            } else if (isRightClick) {
                XTestFakeButtonEvent(m_display, RightMouseButton, true, CurrentTime);
                XTestFakeButtonEvent(m_display, RightMouseButton, false, CurrentTime);
            } else if( isScroll ) {
                if (dy < 0) {
                    XTestFakeButtonEvent(m_display, MouseWheelDown, true, CurrentTime);
                    XTestFakeButtonEvent(m_display, MouseWheelDown, false, CurrentTime);
                } else if (dy > 0) {
                    XTestFakeButtonEvent(m_display, MouseWheelUp, true, CurrentTime);
                    XTestFakeButtonEvent(m_display, MouseWheelUp, false, CurrentTime);
                }
            }
            XFlush(m_display);
        }
    } else {
        QPoint point = QCursor::pos();
        QCursor::setPos(point.x() + (int)dx, point.y() + (int)dy);
    }
    return true;
}
