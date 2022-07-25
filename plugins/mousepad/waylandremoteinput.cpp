/**
 * SPDX-FileCopyrightText: 2015 Martin Gräßlin <mgraesslin@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "waylandremoteinput.h"

#include <QDebug>
#include <QGuiApplication>
#include <QSizeF>
#include <QTemporaryFile>

#include <KLocalizedString>

#include <qpa/qplatformnativeinterface.h>
#include <QtWaylandClient/qwaylandclientextension.h>
#include "qwayland-fake-input.h"
#include "qwayland-wayland.h"
#include "qwayland-wayland.h"

#include <linux/input.h>
#include <xkbcommon/xkbcommon.h>
#include <unistd.h>
#include <sys/mman.h>

namespace
{
//Translation table to keep in sync within all the implementations
int SpecialKeysMap[] = {
    0,               // Invalid
    KEY_BACKSPACE,   // 1
    KEY_TAB,         // 2
    KEY_LINEFEED,    // 3
    KEY_LEFT,        // 4
    KEY_UP,          // 5
    KEY_RIGHT,       // 6
    KEY_DOWN,        // 7
    KEY_PAGEUP,      // 8
    KEY_PAGEDOWN,    // 9
    KEY_HOME,        // 10
    KEY_END,         // 11
    KEY_ENTER,       // 12
    KEY_DELETE,      // 13
    KEY_ESC,         // 14
    KEY_SYSRQ,       // 15
    KEY_SCROLLLOCK,  // 16
    0,               // 17
    0,               // 18
    0,               // 19
    0,               // 20
    KEY_F1,          // 21
    KEY_F2,          // 22
    KEY_F3,          // 23
    KEY_F4,          // 24
    KEY_F5,          // 25
    KEY_F6,          // 26
    KEY_F7,          // 27
    KEY_F8,          // 28
    KEY_F9,          // 29
    KEY_F10,         // 30
    KEY_F11,         // 31
    KEY_F12,         // 32
};
}

class FakeInput : public QWaylandClientExtensionTemplate<FakeInput>, public QtWayland::org_kde_kwin_fake_input
{
public:
    FakeInput()
        : QWaylandClientExtensionTemplate<FakeInput>(5)
    {
    }
};

class WlKeyboard : public QtWayland::wl_keyboard
{
public:
    WlKeyboard(WaylandRemoteInput *input)
        : m_input(input)
    {}

    void keyboard_keymap(uint32_t format, int32_t fd, uint32_t size) override {
        if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
            close(fd);
            return;
        }

        char *map_str = static_cast<char *>(mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0));
        if (map_str == MAP_FAILED) {
            close(fd);
            return;
        }

        m_input->setKeymap(QByteArray(map_str, size));
    }
    WaylandRemoteInput *const m_input;
};

WaylandRemoteInput::WaylandRemoteInput(QObject* parent)
    : AbstractRemoteInput(parent)
    , m_fakeInput(new FakeInput)
    , m_keyboard(new WlKeyboard(this))
    , m_waylandAuthenticationRequested(false)
{
    m_keyboard->init(static_cast<wl_keyboard *>(QGuiApplication::platformNativeInterface()->nativeResourceForIntegration("wl_keyboard")));
}

WaylandRemoteInput::~WaylandRemoteInput()
{
}

bool WaylandRemoteInput::handlePacket(const NetworkPacket& np)
{
    if (!m_fakeInput->isActive()) {
        return true;
    }

    if (!m_waylandAuthenticationRequested) {
        m_fakeInput->authenticate(i18n("KDE Connect"), i18n("Use your phone as a touchpad and keyboard"));
        m_waylandAuthenticationRequested = true;
    }

    const float dx = np.get<float>(QStringLiteral("dx"), 0);
    const float dy = np.get<float>(QStringLiteral("dy"), 0);

    const bool isSingleClick = np.get<bool>(QStringLiteral("singleclick"), false);
    const bool isDoubleClick = np.get<bool>(QStringLiteral("doubleclick"), false);
    const bool isMiddleClick = np.get<bool>(QStringLiteral("middleclick"), false);
    const bool isRightClick = np.get<bool>(QStringLiteral("rightclick"), false);
    const bool isSingleHold = np.get<bool>(QStringLiteral("singlehold"), false);
    const bool isSingleRelease = np.get<bool>(QStringLiteral("singlerelease"), false);
    const bool isScroll = np.get<bool>(QStringLiteral("scroll"), false);
    const QString key = np.get<QString>(QStringLiteral("key"), QLatin1String(""));
    const int specialKey = np.get<int>(QStringLiteral("specialKey"), 0);

    if (isSingleClick || isDoubleClick || isMiddleClick || isRightClick || isSingleHold || isSingleRelease || isScroll || !key.isEmpty() || specialKey) {
        if (isSingleClick) {
            m_fakeInput->button(BTN_LEFT, WL_POINTER_BUTTON_STATE_PRESSED);
            m_fakeInput->button(BTN_LEFT, WL_POINTER_BUTTON_STATE_RELEASED);

        } else if (isDoubleClick) {
            m_fakeInput->button(BTN_LEFT, WL_POINTER_BUTTON_STATE_PRESSED);
            m_fakeInput->button(BTN_LEFT, WL_POINTER_BUTTON_STATE_RELEASED);

            m_fakeInput->button(BTN_LEFT, WL_POINTER_BUTTON_STATE_PRESSED);
            m_fakeInput->button(BTN_LEFT, WL_POINTER_BUTTON_STATE_RELEASED);
        } else if (isMiddleClick) {
            m_fakeInput->button(BTN_MIDDLE, WL_POINTER_BUTTON_STATE_PRESSED);
            m_fakeInput->button(BTN_MIDDLE, WL_POINTER_BUTTON_STATE_RELEASED);
        } else if (isRightClick) {
            m_fakeInput->button(BTN_RIGHT, WL_POINTER_BUTTON_STATE_PRESSED);
            m_fakeInput->button(BTN_RIGHT, WL_POINTER_BUTTON_STATE_RELEASED);
        } else if (isSingleHold){
            //For drag'n drop
            m_fakeInput->button(BTN_LEFT, WL_POINTER_BUTTON_STATE_PRESSED);
        } else if (isSingleRelease){
            //For drag'n drop. NEVER USED (release is done by tapping, which actually triggers a isSingleClick). Kept here for future-proofness.
            m_fakeInput->button(BTN_LEFT, WL_POINTER_BUTTON_STATE_RELEASED);
        } else if (isScroll) {
            m_fakeInput->axis(WL_POINTER_AXIS_VERTICAL_SCROLL, wl_fixed_from_double(-dy));
        } else if (specialKey) {
            m_fakeInput->keyboard_key(SpecialKeysMap[specialKey], 1);
            m_fakeInput->keyboard_key(SpecialKeysMap[specialKey], 0);
        } else if (!key.isEmpty()) {
            if (!m_keymapSent && !m_keymap.isEmpty()) {
                m_keymapSent = true;
                auto sendKeymap = [this] {
                    QScopedPointer<QTemporaryFile> tmp(new QTemporaryFile());
                    if (!tmp->open()) {
                        return;
                    }

                    unlink(tmp->fileName().toUtf8().constData());
                    if (!tmp->resize(m_keymap.size())) {
                        return;
                    }

                    uchar *address = tmp->map(0, m_keymap.size());
                    if (!address) {
                        return;
                    }

                    qstrncpy(reinterpret_cast<char *>(address), m_keymap.constData(), m_keymap.size() + 1);
                    tmp->unmap(address);

                    m_fakeInput->keyboard_keymap(tmp->handle(), tmp->size());
                };
                sendKeymap();
            }

            for (const QChar character : key) {
                const auto keysym = xkb_utf32_to_keysym(character.unicode());
                m_fakeInput->keyboard_keysym(keysym, 1);
                m_fakeInput->keyboard_keysym(keysym, 0);
            }
        }
    } else { //Is a mouse move event
        m_fakeInput->pointer_motion(wl_fixed_from_double(dx), wl_fixed_from_double(dy));
    }
    return true;
}
