/**
 * SPDX-FileCopyrightText: 2018 Albert Vaca Cintora <albertvaka@gmail.com>
 * SPDX-FileCopyrightText: 2014 Ahmed I. Khalil <ahmedibrahimkhali@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "x11remoteinput.h"

#include <QCursor>
#include <QDebug>
#include <private/qtx11extras_p.h>

#include <X11/extensions/XTest.h>
#include <X11/keysym.h>
#include <fakekey/fakekey.h>

enum MouseButtons {
    LeftMouseButton = 1,
    MiddleMouseButton = 2,
    RightMouseButton = 3,
    MouseWheelUp = 4,
    MouseWheelDown = 5,
};

// Translation table to keep in sync within all the implementations
int SpecialKeysMap[] = {
    0, // Invalid
    XK_BackSpace, // 1
    XK_Tab, // 2
    XK_Linefeed, // 3
    XK_Left, // 4
    XK_Up, // 5
    XK_Right, // 6
    XK_Down, // 7
    XK_Page_Up, // 8
    XK_Page_Down, // 9
    XK_Home, // 10
    XK_End, // 11
    XK_Return, // 12
    XK_Delete, // 13
    XK_Escape, // 14
    XK_Sys_Req, // 15
    XK_Scroll_Lock, // 16
    0, // 17
    0, // 18
    0, // 19
    0, // 20
    XK_F1, // 21
    XK_F2, // 22
    XK_F3, // 23
    XK_F4, // 24
    XK_F5, // 25
    XK_F6, // 26
    XK_F7, // 27
    XK_F8, // 28
    XK_F9, // 29
    XK_F10, // 30
    XK_F11, // 31
    XK_F12, // 32
};

template<typename T, size_t N>
size_t arraySize(T (&arr)[N])
{
    (void)arr;
    return N;
}

X11RemoteInput::X11RemoteInput(QObject *parent)
    : AbstractRemoteInput(parent)
    , m_fakekey(nullptr)
{
}

X11RemoteInput::~X11RemoteInput()
{
    if (m_fakekey) {
        free(m_fakekey);
        m_fakekey = nullptr;
    }
}

bool isLeftHanded(Display *display)
{
    unsigned char map[20];
    int num_buttons = XGetPointerMapping(display, map, 20);
    if (num_buttons == 1) {
        return false;
    } else if (num_buttons == 2) {
        return ((int)map[0] == 2 && (int)map[1] == 1);
    } else {
        return ((int)map[0] == 3 && (int)map[2] == 1);
    }
}

bool X11RemoteInput::handlePacket(const NetworkPacket &np)
{
    float dx = np.get<float>(QStringLiteral("dx"), 0);
    float dy = np.get<float>(QStringLiteral("dy"), 0);
    float x = np.get<float>(QStringLiteral("x"), 0);
    float y = np.get<float>(QStringLiteral("y"), 0);

    bool isSingleClick = np.get<bool>(QStringLiteral("singleclick"), false);
    bool isDoubleClick = np.get<bool>(QStringLiteral("doubleclick"), false);
    bool isMiddleClick = np.get<bool>(QStringLiteral("middleclick"), false);
    bool isRightClick = np.get<bool>(QStringLiteral("rightclick"), false);
    bool isSingleHold = np.get<bool>(QStringLiteral("singlehold"), false);
    bool isSingleRelease = np.get<bool>(QStringLiteral("singlerelease"), false);
    bool isScroll = np.get<bool>(QStringLiteral("scroll"), false);
    QString key = np.get<QString>(QStringLiteral("key"), QLatin1String(""));
    int specialKey = np.get<int>(QStringLiteral("specialKey"), 0);

    if (isSingleClick || isDoubleClick || isMiddleClick || isRightClick || isSingleHold || isSingleRelease || isScroll || !key.isEmpty() || specialKey) {
        Display *display = QX11Info::display();
        if (!display) {
            return false;
        }

        bool leftHanded = isLeftHanded(display);
        int mainMouseButton = leftHanded ? RightMouseButton : LeftMouseButton;
        int secondaryMouseButton = leftHanded ? LeftMouseButton : RightMouseButton;

        if (isSingleClick) {
            XTestFakeButtonEvent(display, mainMouseButton, True, 0);
            XTestFakeButtonEvent(display, mainMouseButton, False, 0);
        } else if (isDoubleClick) {
            XTestFakeButtonEvent(display, mainMouseButton, True, 0);
            XTestFakeButtonEvent(display, mainMouseButton, False, 0);
            XTestFakeButtonEvent(display, mainMouseButton, True, 0);
            XTestFakeButtonEvent(display, mainMouseButton, False, 0);
        } else if (isMiddleClick) {
            XTestFakeButtonEvent(display, MiddleMouseButton, True, 0);
            XTestFakeButtonEvent(display, MiddleMouseButton, False, 0);
        } else if (isRightClick) {
            XTestFakeButtonEvent(display, secondaryMouseButton, True, 0);
            XTestFakeButtonEvent(display, secondaryMouseButton, False, 0);
        } else if (isSingleHold) {
            // For drag'n drop
            XTestFakeButtonEvent(display, mainMouseButton, True, 0);
        } else if (isSingleRelease) {
            XTestFakeButtonEvent(display, mainMouseButton, False, 0);
        } else if (isScroll) {
            if (dy < 0) {
                XTestFakeButtonEvent(display, MouseWheelDown, True, 0);
                XTestFakeButtonEvent(display, MouseWheelDown, False, 0);
            } else if (dy > 0) {
                XTestFakeButtonEvent(display, MouseWheelUp, True, 0);
                XTestFakeButtonEvent(display, MouseWheelUp, False, 0);
            }
        } else if (!key.isEmpty() || specialKey) {
            bool ctrl = np.get<bool>(QStringLiteral("ctrl"), false);
            bool alt = np.get<bool>(QStringLiteral("alt"), false);
            bool shift = np.get<bool>(QStringLiteral("shift"), false);
            bool super = np.get<bool>(QStringLiteral("super"), false);

            if (ctrl)
                XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Control_L), True, 0);
            if (alt)
                XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Alt_L), True, 0);
            if (shift)
                XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Shift_L), True, 0);
            if (super)
                XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Super_L), True, 0);

            if (specialKey) {
                if (specialKey >= (int)arraySize(SpecialKeysMap)) {
                    qWarning() << "Unsupported special key identifier";
                    return false;
                }

                int keycode = XKeysymToKeycode(display, SpecialKeysMap[specialKey]);

                XTestFakeKeyEvent(display, keycode, True, 0);
                XTestFakeKeyEvent(display, keycode, False, 0);

            } else {
                if (!m_fakekey) {
                    m_fakekey = fakekey_init(display);
                    if (!m_fakekey) {
                        qWarning() << "Failed to initialize libfakekey";
                        return false;
                    }
                }

                // We use fakekey here instead of XTest (above) because it can handle utf characters instead of keycodes.
                for (int i = 0; i < key.length(); i++) {
                    QByteArray utf8 = QString(key.at(i)).toUtf8();
                    fakekey_press(m_fakekey, (const uchar *)utf8.constData(), utf8.size(), 0);
                    fakekey_release(m_fakekey);
                }
            }

            if (ctrl)
                XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Control_L), False, 0);
            if (alt)
                XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Alt_L), False, 0);
            if (shift)
                XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Shift_L), False, 0);
            if (super)
                XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Super_L), False, 0);
        }

        XFlush(display);

    } else { // Is a mouse move event
        if (dx || dy) {
            QPoint point = QCursor::pos();
            QCursor::setPos(point.x() + (int)dx, point.y() + (int)dy);
        } else if (np.has(QStringLiteral("x")) || np.has(QStringLiteral("y"))) {
            QCursor::setPos(x, y);
        }
    }
    return true;
}

bool X11RemoteInput::hasKeyboardSupport()
{
    return true;
}

#include "moc_x11remoteinput.cpp"
