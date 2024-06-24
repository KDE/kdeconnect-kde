/**
 * SPDX-FileCopyrightText: 2019 Weixuan XIAO <veyx.shaw@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "macosremoteinput.h"

#include <QCursor>
#include <QDebug>

#include <CoreGraphics/CGEvent.h>
#include <Carbon/Carbon.h>
#include <ApplicationServices/ApplicationServices.h>

int SpecialKeysMap[] = {
    0,                  // Invalid
    kVK_Delete,         // 1
    kVK_Tab,            // 2
    kVK_Return,         // 3
    kVK_LeftArrow,      // 4
    kVK_UpArrow,        // 5
    kVK_RightArrow,     // 6
    kVK_DownArrow,      // 7
    kVK_PageUp,         // 8
    kVK_PageDown,       // 9
    kVK_Home,           // 10
    kVK_End,            // 11
    kVK_Return,         // 12
    kVK_ForwardDelete,  // 13
    kVK_Escape,         // 14
    0,                  // 15
    0,                  // 16
    0,                  // 17
    0,                  // 18
    0,                  // 19
    0,                  // 20
    kVK_F1,             // 21
    kVK_F2,             // 22
    kVK_F3,             // 23
    kVK_F4,             // 24
    kVK_F5,             // 25
    kVK_F6,             // 26
    kVK_F7,             // 27
    kVK_F8,             // 28
    kVK_F9,             // 29
    kVK_F10,            // 30
    kVK_F11,            // 31
    kVK_F12,            // 32
};

template <typename T, size_t N>
size_t arraySize(T(&arr)[N]) { (void)arr; return N; }

MacOSRemoteInput::MacOSRemoteInput(QObject* parent)
    : AbstractRemoteInput(parent)
{
    // Ask user to enable accessibility API
    NSDictionary *options = @{(id)kAXTrustedCheckOptionPrompt: @YES};
    if (!AXIsProcessTrustedWithOptions((CFDictionaryRef)options)) {
        qWarning() << "kdeconnectd is not set as a trusted accessibility, mousepad and keyboard will not work";
    }
}

bool MacOSRemoteInput::handlePacket(const NetworkPacket& np)
{
    // Return if not trusted
    if (!AXIsProcessTrusted()) {
        return false;
    }

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
        QPoint point = QCursor::pos();

        if (isSingleClick) {
            CGEventRef event = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, CGPointMake(point.x(), point.y()), kCGMouseButtonLeft);
            CGEventPost(kCGHIDEventTap, event);
            CGEventSetType(event, kCGEventLeftMouseUp);
            CGEventPost(kCGHIDEventTap, event);
            CFRelease(event);
        } else if (isDoubleClick) {
            CGEventRef event = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, CGPointMake(point.x(), point.y()), kCGMouseButtonLeft);
            CGEventPost(kCGHIDEventTap, event);
            CGEventSetType(event, kCGEventLeftMouseUp);
            CGEventPost(kCGHIDEventTap, event);

            /* Key to access an integer field that contains the mouse button click
            state. A click state of 1 represents a single click. A click state of 2
            represents a double-click. A click state of 3 represents a
            triple-click. */
            CGEventSetIntegerValueField(event, kCGMouseEventClickState, 2);

            CGEventSetType(event, kCGEventLeftMouseDown);
            CGEventPost(kCGHIDEventTap, event);
            CGEventSetType(event, kCGEventLeftMouseUp);
            CGEventPost(kCGHIDEventTap, event);
            CFRelease(event);
		} else if (isMiddleClick) {
            CGEventRef event = CGEventCreateMouseEvent(NULL, kCGEventOtherMouseDown, CGPointMake(point.x(), point.y()), kCGMouseButtonLeft);
            CGEventPost(kCGHIDEventTap, event);
            CGEventSetType(event, kCGEventOtherMouseUp);
            CGEventPost(kCGHIDEventTap, event);
            CFRelease(event);
		} else if (isRightClick) {
            CGEventRef event = CGEventCreateMouseEvent(NULL, kCGEventRightMouseDown, CGPointMake(point.x(), point.y()), kCGMouseButtonLeft);
            CGEventPost(kCGHIDEventTap, event);
            CGEventSetType(event, kCGEventRightMouseUp);
            CGEventPost(kCGHIDEventTap, event);
            CFRelease(event);
		} else if (isSingleHold) {
            CGEventRef event = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, CGPointMake(point.x(), point.y()), kCGMouseButtonLeft);
            CGEventPost(kCGHIDEventTap, event);
            CFRelease(event);
        } else if (isSingleRelease) {
            CGEventRef event = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp, CGPointMake(point.x(), point.y()), kCGMouseButtonLeft);
            CGEventPost(kCGHIDEventTap, event);
            CFRelease(event);
        } else if (isScroll) {
            CGEventRef event = CGEventCreateScrollWheelEvent(NULL, kCGScrollEventUnitPixel, 1, dy);
            CGEventPost(kCGHIDEventTap, event);
            CFRelease(event);
        } else if (!key.isEmpty() || specialKey) {
            // Get function keys
            bool ctrl = np.get<bool>(QStringLiteral("ctrl"), false);
            bool alt = np.get<bool>(QStringLiteral("alt"), false);
            bool shift = np.get<bool>(QStringLiteral("shift"), false);
            bool super = np.get<bool>(QStringLiteral("super"), false);


            if (ctrl) {
                CGEventRef ctrlEvent = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)kVK_Control, true);
                CGEventPost(kCGHIDEventTap, ctrlEvent);
                CFRelease(ctrlEvent);
            }
            if (alt) {
                CGEventRef optionEvent = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)kVK_Option, true);
                CGEventPost(kCGHIDEventTap, optionEvent);
                CFRelease(optionEvent);
            }
            if (shift) {
                CGEventRef shiftEvent = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)kVK_Shift, true);
                CGEventPost(kCGHIDEventTap, shiftEvent);
                CFRelease(shiftEvent);
            }
            if (super) {
                CGEventRef commandEvent = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)kVK_Command, true);
                CGEventPost(kCGHIDEventTap, commandEvent);
                CFRelease(commandEvent);
            }

            // Keys
            if (specialKey)
            {
                if (specialKey >= (int)arraySize(SpecialKeysMap)) {
                    qWarning() << "Unsupported special key identifier";
                    return false;
                }

                CGEventRef specialKeyDownEvent = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)SpecialKeysMap[specialKey], true),
                           specialKeyUpEvent = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)SpecialKeysMap[specialKey], false);

                CGEventPost(kCGHIDEventTap, specialKeyDownEvent);
                CGEventPost(kCGHIDEventTap, specialKeyUpEvent);

                CFRelease(specialKeyDownEvent);
                CFRelease(specialKeyUpEvent);
            } else {
                for (int i = 0; i < key.length(); i++) {
                    const UniChar unicharData = (const UniChar)key.at(i).unicode();

                    CGEventRef event = CGEventCreateKeyboardEvent(NULL, 0, true);

                    CGEventKeyboardSetUnicodeString(event, 1, &unicharData);
                    CGEventPost(kCGSessionEventTap, event);

                    CFRelease(event);
                }

            }

            if (ctrl) {
                CGEventRef ctrlEvent = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)kVK_Control, false);
                CGEventPost(kCGHIDEventTap, ctrlEvent);
                CFRelease(ctrlEvent);
            }
            if (alt) {
                CGEventRef optionEvent = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)kVK_Option, false);
                CGEventPost(kCGHIDEventTap, optionEvent);
                CFRelease(optionEvent);
            }
            if (shift) {
                CGEventRef shiftEvent = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)kVK_Shift, false);
                CGEventPost(kCGHIDEventTap, shiftEvent);
                CFRelease(shiftEvent);
            }
            if (super) {
                CGEventRef commandEvent = CGEventCreateKeyboardEvent(NULL, (CGKeyCode)kVK_Command, false);
                CGEventPost(kCGHIDEventTap, commandEvent);
                CFRelease(commandEvent);
            }

        }
    } else { //Is a mouse move event
         if (dx || dy) {
            QPoint point = QCursor::pos();
            QCursor::setPos(point.x() + (int)dx, point.y() + (int)dy);
        } else if (np.has(QStringLiteral("x")) || np.has(QStringLiteral("y"))) {
            QCursor::setPos(x, y);
        }
    }
    return true;
}

bool MacOSRemoteInput::hasKeyboardSupport() {
    return true;
}

#include "moc_macosremoteinput.cpp"
