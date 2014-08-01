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
#include <X11/keysym.h>

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

XKeyEvent MousepadPlugin::createKeyEvent(Display *display, Window &win, Window &winRoot, bool press, KeyCode keycode, int modifiers)
{
       // http://www.doctort.org/adam/nerd-notes/x11-fake-keypress-event.html
       XKeyEvent event;

       event.display = display;
       event.window = win;
       event.root = winRoot;
       event.subwindow = None;
       event.time = CurrentTime;
       event.x = 1;
       event.y = 1;
       event.x_root = 1;
       event.y_root = 1;
       event.same_screen = true;
       event.keycode = keycode;
       event.state = modifiers;

       if (press) {
           event.type = KeyPress;
       } else {
           event.type = KeyRelease;
       }

       return event;
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
    QString key = np.get<QString>("key", "");
    int modifiers = np.get<int>("modifiers", 0);

    if (isSingleClick || isDoubleClick || isMiddleClick || isRightClick || isScroll || !key.isEmpty() || modifiers) {
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
            } else if (!key.isEmpty() || modifiers) {
                Window winRoot = XDefaultRootWindow(m_display);

                Window winFocus;
                int revert;
                XGetInputFocus(m_display, &winFocus, &revert);

                KeyCode keycode = 0;
                int xModifier = 0;
                if (!key.isEmpty()) {
                    if (key == QLatin1String(".")) {
                        keycode = XKeysymToKeycode(m_display, XK_period);
                    } else if (key == QLatin1String(",")) {
                        keycode = XKeysymToKeycode(m_display, XK_comma);
                    } else {
                        QByteArray keyByteArray = key.toLocal8Bit();
                        KeySym keysym = XStringToKeysym(keyByteArray.constData());
                        keycode = XKeysymToKeycode(m_display, keysym);
                    }
                } else {
                    // It's a special key
                    if ((modifiers & FunctionalKeys) == Backspace) {
                        keycode = XKeysymToKeycode(m_display, XK_BackSpace);
                    } else if ((modifiers & FunctionalKeys) == Enter) {
                        keycode = XKeysymToKeycode(m_display, XK_Linefeed);
                    } else if ((modifiers & FunctionalKeys) == Tab) {
                        keycode = XKeysymToKeycode(m_display, XK_Tab);
                    }
                }

                if (modifiers & Shift) {
                    xModifier |= ShiftMask;
                }
                if (modifiers & Control) {
                    xModifier |= ControlMask;
                }

                XKeyEvent event = createKeyEvent(m_display, winFocus, winRoot, true, keycode, xModifier);
                XSendEvent(event.display, event.window, true, KeyPressMask, (XEvent*)&event);

                event = createKeyEvent(m_display, winFocus, winRoot, false, keycode, xModifier);
                XSendEvent(event.display, event.window, true, KeyPressMask, (XEvent*)&event);
            }
            XFlush(m_display);
        }
    } else {
        QPoint point = QCursor::pos();
        QCursor::setPos(point.x() + (int)dx, point.y() + (int)dy);
    }
    return true;
}
