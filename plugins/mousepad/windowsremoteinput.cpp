/**
 * SPDX-FileCopyrightText: 2018 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "windowsremoteinput.h"

#include <QCursor>
#include <QDebug>

#include <Windows.h>

// Translation table to keep in sync within all the implementations
int SpecialKeysMap[] = {
    0, // Invalid
    VK_BACK, // 1
    VK_TAB, // 2
    VK_RETURN, // 3
    VK_LEFT, // 4
    VK_UP, // 5
    VK_RIGHT, // 6
    VK_DOWN, // 7
    VK_PRIOR, // 8
    VK_NEXT, // 9
    VK_HOME, // 10
    VK_END, // 11
    VK_RETURN, // 12
    VK_DELETE, // 13
    VK_ESCAPE, // 14
    VK_SNAPSHOT, // 15
    VK_SCROLL, // 16
    0, // 17
    0, // 18
    0, // 19
    0, // 20
    VK_F1, // 21
    VK_F2, // 22
    VK_F3, // 23
    VK_F4, // 24
    VK_F5, // 25
    VK_F6, // 26
    VK_F7, // 27
    VK_F8, // 28
    VK_F9, // 29
    VK_F10, // 30
    VK_F11, // 31
    VK_F12, // 32
};

template<typename T, size_t N>
size_t arraySize(T (&arr)[N])
{
    (void)arr;
    return N;
}

WindowsRemoteInput::WindowsRemoteInput(QObject *parent)
    : AbstractRemoteInput(parent)
{
}

bool WindowsRemoteInput::handlePacket(const NetworkPacket &np)
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

    if (isSingleClick || isDoubleClick || isMiddleClick || isRightClick || isSingleHold || isScroll || isSingleRelease || !key.isEmpty() || specialKey) {
        INPUT input = {0};
        input.type = INPUT_MOUSE;

        if (isSingleClick) {
            input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            ::SendInput(1, &input, sizeof(INPUT));
            input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            ::SendInput(1, &input, sizeof(INPUT));
        } else if (isDoubleClick) {
            input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            ::SendInput(1, &input, sizeof(INPUT));
            input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            ::SendInput(1, &input, sizeof(INPUT));
            input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            ::SendInput(1, &input, sizeof(INPUT));
            input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            ::SendInput(1, &input, sizeof(INPUT));
        } else if (isMiddleClick) {
            input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
            ::SendInput(1, &input, sizeof(INPUT));
            input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
            ::SendInput(1, &input, sizeof(INPUT));
        } else if (isRightClick) {
            input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
            ::SendInput(1, &input, sizeof(INPUT));
            input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
            ::SendInput(1, &input, sizeof(INPUT));
        } else if (isSingleHold) {
            input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            ::SendInput(1, &input, sizeof(INPUT));
        } else if (isSingleRelease) {
            input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            ::SendInput(1, &input, sizeof(INPUT));
        } else if (isScroll) {
            input.mi.dwFlags = MOUSEEVENTF_WHEEL;
            input.mi.mouseData = dy;
            ::SendInput(1, &input, sizeof(INPUT));

        } else if (!key.isEmpty() || specialKey) {
            input.type = INPUT_KEYBOARD;

            input.ki.time = 0;
            input.ki.dwExtraInfo = 0;
            input.ki.wScan = 0;
            input.ki.dwFlags = 0;

            bool ctrl = np.get<bool>(QStringLiteral("ctrl"), false);
            bool alt = np.get<bool>(QStringLiteral("alt"), false);
            bool shift = np.get<bool>(QStringLiteral("shift"), false);
            bool super = np.get<bool>(QStringLiteral("super"), false);

            if (ctrl) {
                input.ki.wVk = VK_LCONTROL;
                ::SendInput(1, &input, sizeof(INPUT));
            }
            if (alt) {
                input.ki.wVk = VK_LMENU;
                ::SendInput(1, &input, sizeof(INPUT));
            }
            if (shift) {
                input.ki.wVk = VK_LSHIFT;
                ::SendInput(1, &input, sizeof(INPUT));
            }
            if (super) {
                input.ki.wVk = VK_LWIN;
                ::SendInput(1, &input, sizeof(INPUT));
            }

            if (specialKey) {
                if (specialKey >= (int)arraySize(SpecialKeysMap)) {
                    qWarning() << "Unsupported special key identifier";
                    return false;
                }

                input.ki.wVk = SpecialKeysMap[specialKey];
                ::SendInput(1, &input, sizeof(INPUT));

                input.ki.dwFlags = KEYEVENTF_KEYUP;
                ::SendInput(1, &input, sizeof(INPUT));

            } else {
                for (int i = 0; i < key.length(); i++) {
                    wchar_t inputChar = (wchar_t)key.at(i).unicode();
                    short inputVk = VkKeyScanExW(inputChar, GetKeyboardLayout(0));

                    if (inputVk != -1) {
                        // Uses virtual keycodes so key combinations work
                        input.ki.wScan = 0;
                        input.ki.dwFlags = 0;

                        if (inputVk & 0x100) {
                            input.ki.wVk = VK_LSHIFT;
                            ::SendInput(1, &input, sizeof(INPUT));
                        }
                        if (inputVk & 0x200) {
                            input.ki.wVk = VK_LCONTROL;
                            ::SendInput(1, &input, sizeof(INPUT));
                        }
                        if (inputVk & 0x400) {
                            input.ki.wVk = VK_LMENU;
                            ::SendInput(1, &input, sizeof(INPUT));
                        }

                        input.ki.wVk = inputVk & 0xFF;
                        ::SendInput(1, &input, sizeof(INPUT));

                        input.ki.dwFlags = KEYEVENTF_KEYUP;
                        ::SendInput(1, &input, sizeof(INPUT));

                        if ((inputVk & 0x100) && !shift) {
                            input.ki.wVk = VK_LSHIFT;
                            ::SendInput(1, &input, sizeof(INPUT));
                        }
                        if ((inputVk & 0x200) && !ctrl) {
                            input.ki.wVk = VK_LCONTROL;
                            ::SendInput(1, &input, sizeof(INPUT));
                        }
                        if ((inputVk & 0x400) && !alt) {
                            input.ki.wVk = VK_LMENU;
                            ::SendInput(1, &input, sizeof(INPUT));
                        }

                    } else {
                        // Falls back to KEYEVENTF_UNICODE
                        input.ki.wVk = 0;
                        input.ki.wScan = inputChar;
                        input.ki.dwFlags = KEYEVENTF_UNICODE;
                        ::SendInput(1, &input, sizeof(INPUT));

                        input.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_UNICODE;
                        ::SendInput(1, &input, sizeof(INPUT));
                    }
                }
            }

            input.ki.dwFlags = KEYEVENTF_KEYUP;
            input.ki.wScan = 0;

            if (ctrl) {
                input.ki.wVk = VK_LCONTROL;
                ::SendInput(1, &input, sizeof(INPUT));
            }
            if (alt) {
                input.ki.wVk = VK_LMENU;
                ::SendInput(1, &input, sizeof(INPUT));
            }
            if (shift) {
                input.ki.wVk = VK_LSHIFT;
                ::SendInput(1, &input, sizeof(INPUT));
            }
            if (super) {
                input.ki.wVk = VK_LWIN;
                ::SendInput(1, &input, sizeof(INPUT));
            }
        }

    } else { // Is a mouse move event
        if (dx || dy) {
            POINT point;
            if (GetCursorPos(&point)) {
                return SetCursorPos(point.x + (int)dx, point.y + (int)dy);
            } else {
                return false;
            }
        } else if ((np.has(QStringLiteral("x")) || np.has(QStringLiteral("y")))) {
            return SetCursorPos((int)x, (int)y);
        }
    }
    return true;
}

bool WindowsRemoteInput::hasKeyboardSupport()
{
    return true;
}

#include "moc_windowsremoteinput.cpp"
