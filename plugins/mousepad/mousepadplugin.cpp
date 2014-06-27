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
#include <QApplication>
#include <X11/extensions/XTest.h>

K_PLUGIN_FACTORY( KdeConnectPluginFactory, registerPlugin< MousepadPlugin >(); )
K_EXPORT_PLUGIN( KdeConnectPluginFactory("kdeconnect_mousepad", "kdeconnect-plugins") )

#define LEFT_MOUSE_BUTTON 1 // Source: http://bharathisubramanian.wordpress.com/2010/04/01/x11-fake-mouse-events-generation-using-xtest/

MousepadPlugin::MousepadPlugin(QObject* parent, const QVariantList& args)
       : KdeConnectPlugin(parent, args)
{
    
}

bool MousepadPlugin::receivePackage(const NetworkPackage& np)
{
    float dx = np.get<float>("dx", 0);
    float dy = np.get<float>("dy", 0);
    QPoint point = QCursor::pos();
    QCursor::setPos(point.x() + (int)dx, point.y() + (int)dy);
    
    bool isSingleClick = np.get<bool>("singleclick", false);
    bool isDoubleClick = np.get<bool>("doubleclick", false);
    
    if (isSingleClick || isDoubleClick) {
	Display *display = XOpenDisplay(NULL);
	if(display) {
	    if (isSingleClick) {
		XTestFakeButtonEvent(display, LEFT_MOUSE_BUTTON, true, CurrentTime);
		XTestFakeButtonEvent(display, LEFT_MOUSE_BUTTON, false, CurrentTime);
	    } else if (isDoubleClick) {
		XTestFakeButtonEvent(display, LEFT_MOUSE_BUTTON, true, CurrentTime);
		XTestFakeButtonEvent(display, LEFT_MOUSE_BUTTON, false, CurrentTime);
		XTestFakeButtonEvent(display, LEFT_MOUSE_BUTTON, true, CurrentTime);
		XTestFakeButtonEvent(display, LEFT_MOUSE_BUTTON, false, CurrentTime);
	    }
	    XFlush(display);
	}
	XCloseDisplay(display);
    }
    return true;
}
