/**
 * SPDX-FileCopyrightText: 2025 James Padgett <snotacusnexus@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "virtualkeyboardinjector.h"

#include <QDebug>
#include <QGuiApplication>
#include <QProcess>
#include <QTime>

#include <linux/input.h>
#include <xkbcommon/xkbcommon.h>

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <wayland-client.h>

#include "wayland-virtual-keyboard-client-protocol.h"

// Same key mapping as used by the mousepad plugin, matching all existing implementations
static const int SpecialKeysMap[] = {
    0, // 0: Invalid
    KEY_BACKSPACE, // 1
    KEY_TAB, // 2
    KEY_LINEFEED, // 3
    KEY_LEFT, // 4
    KEY_UP, // 5
    KEY_RIGHT, // 6
    KEY_DOWN, // 7
    KEY_PAGEUP, // 8
    KEY_PAGEDOWN, // 9
    KEY_HOME, // 10
    KEY_END, // 11
    KEY_ENTER, // 12
    KEY_DELETE, // 13
    KEY_ESC, // 14
    KEY_SYSRQ, // 15
    KEY_SCROLLLOCK, // 16
    0, // 17
    0, // 18
    0, // 19
    0, // 20
    KEY_F1, // 21
    KEY_F2, // 22
    KEY_F3, // 23
    KEY_F4, // 24
    KEY_F5, // 25
    KEY_F6, // 26
    KEY_F7, // 27
    KEY_F8, // 28
    KEY_F9, // 29
    KEY_F10, // 30
    KEY_F11, // 31
    KEY_F12, // 32
};

VirtualKeyboardInjector::VirtualKeyboardInjector(QObject *parent)
    : QObject(parent)
{
    connectToCompositor();
}

VirtualKeyboardInjector::~VirtualKeyboardInjector()
{
    disconnectFromCompositor();
}

bool VirtualKeyboardInjector::isAvailable()
{
    // Quick environment check first
    if (QGuiApplication::platformName() != QLatin1String("wayland")) {
        return false;
    }

    // Open a temporary connection to check for the protocol
    wl_display *display = wl_display_connect(nullptr);
    if (!display) {
        return false;
    }

    struct CheckData {
        bool found = false;
    } data;

    wl_registry *registry = wl_display_get_registry(display);

    auto globalCb = [](void *data, struct wl_registry *, uint32_t, const char *interface, uint32_t) {
        auto *d = static_cast<CheckData *>(data);
        if (strcmp(interface, zwp_virtual_keyboard_manager_v1_interface.name) == 0) {
            d->found = true;
        }
    };

    auto globalRemoveCb = [](void *, struct wl_registry *, uint32_t) { };

    static const wl_registry_listener listener = {globalCb, globalRemoveCb};
    wl_registry_add_listener(registry, &listener, &data);
    wl_display_roundtrip(display);

    wl_registry_destroy(registry);
    wl_display_disconnect(display);

    return data.found;
}

bool VirtualKeyboardInjector::connectToCompositor()
{
    m_display = wl_display_connect(nullptr);
    if (!m_display) {
        qWarning() << "VirtualKeyboardInjector: cannot connect to Wayland display";
        return false;
    }

    m_registry = wl_display_get_registry(m_display);
    if (!m_registry) {
        wl_display_disconnect(m_display);
        m_display = nullptr;
        return false;
    }

    static const wl_registry_listener listener = {sRegistryGlobal, nullptr};
    wl_registry_add_listener(m_registry, &listener, this);
    wl_display_roundtrip(m_display);

    if (!m_keyboardManager || !m_seat) {
        qWarning() << "VirtualKeyboardInjector: compositor doesn't support virtual keyboard protocol";
        wl_registry_destroy(m_registry);
        wl_display_disconnect(m_display);
        m_registry = nullptr;
        m_display = nullptr;
        return false;
    }

    // Create virtual keyboard
    m_virtualKeyboard = zwp_virtual_keyboard_manager_v1_create_virtual_keyboard(m_keyboardManager, m_seat);

    // Create default keymap
    m_xkbCtx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (m_xkbCtx) {
        m_xkbKeymap = xkb_keymap_new_from_names(m_xkbCtx, nullptr, XKB_KEYMAP_COMPILE_NO_FLAGS);
        if (m_xkbKeymap) {
            ensureKeymap();
        }
    }

    m_connected = true;
    return true;
}

void VirtualKeyboardInjector::disconnectFromCompositor()
{
    m_connected = false;

    if (m_virtualKeyboard) {
        zwp_virtual_keyboard_v1_destroy(m_virtualKeyboard);
        m_virtualKeyboard = nullptr;
    }

    if (m_keyboardManager) {
        zwp_virtual_keyboard_manager_v1_destroy(m_keyboardManager);
        m_keyboardManager = nullptr;
    }

    if (m_registry) {
        wl_registry_destroy(m_registry);
        m_registry = nullptr;
    }

    if (m_xkbKeymap) {
        xkb_keymap_unref(m_xkbKeymap);
        m_xkbKeymap = nullptr;
    }

    if (m_xkbCtx) {
        xkb_context_unref(m_xkbCtx);
        m_xkbCtx = nullptr;
    }

    if (m_display) {
        wl_display_flush(m_display);
        wl_display_disconnect(m_display);
        m_display = nullptr;
    }

    m_seat = nullptr;
}

void VirtualKeyboardInjector::sRegistryGlobal(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
    auto *self = static_cast<VirtualKeyboardInjector *>(data);

    if (strcmp(interface, wl_seat_interface.name) == 0) {
        self->m_seat = static_cast<wl_seat *>(wl_registry_bind(registry, name, &wl_seat_interface, std::min(version, 7u)));
    } else if (strcmp(interface, zwp_virtual_keyboard_manager_v1_interface.name) == 0) {
        self->m_keyboardManager =
            static_cast<zwp_virtual_keyboard_manager_v1 *>(wl_registry_bind(registry, name, &zwp_virtual_keyboard_manager_v1_interface, std::min(version, 1u)));
    }
}

void VirtualKeyboardInjector::ensureKeymap()
{
    if (!m_virtualKeyboard || !m_xkbKeymap) {
        return;
    }

    char *keymapString = xkb_keymap_get_as_string(m_xkbKeymap, XKB_KEYMAP_FORMAT_TEXT_V1);
    if (!keymapString) {
        return;
    }

    int size = strlen(keymapString) + 1;

    int fd = memfd_create("kdeconnect-keymap", MFD_CLOEXEC);
    if (fd >= 0) {
        write(fd, keymapString, size);
        lseek(fd, 0, SEEK_SET);
        zwp_virtual_keyboard_v1_keymap(m_virtualKeyboard, XKB_KEYMAP_FORMAT_TEXT_V1, fd, size);
        close(fd);
    }

    free(keymapString);
    wl_display_flush(m_display);
}

uint32_t VirtualKeyboardInjector::keycodeFromKeysym(uint32_t sym)
{
    if (!m_xkbKeymap) {
        return 0;
    }

    xkb_keycode_t min = xkb_keymap_min_keycode(m_xkbKeymap);

    // Search XKB keycodes (which have an offset from evdev scancodes)
    xkb_keycode_t max = xkb_keymap_max_keycode(m_xkbKeymap);
    for (xkb_keycode_t code = min; code <= max; code++) {
        const xkb_keysym_t *syms;
        int num_syms = xkb_keymap_key_get_syms_by_level(m_xkbKeymap, code, 0, 0, &syms);
        for (int i = 0; i < num_syms; i++) {
            if (syms[i] == static_cast<xkb_keysym_t>(sym)) {
                // Convert XKB keycode to evdev scancode.
                // XKB codes start at min_keycode (typically 9), evdev codes start at 1 (KEY_ESC).
                // So offset = min - 1. e.g., XKB 24 → 24 - (9 - 1) = 16 = KEY_Q
                return code - min + 1;
            }
        }
    }
    return 0;
}

uint32_t VirtualKeyboardInjector::nextTimestamp()
{
    uint32_t now = QTime::currentTime().msecsSinceStartOfDay();
    if (now <= m_lastTimestamp) {
        now = m_lastTimestamp + 1;
    }
    m_lastTimestamp = now;
    return now;
}

void VirtualKeyboardInjector::setModifiers(bool shift, bool ctrl, bool alt, bool super)
{
    if (!m_virtualKeyboard || !m_xkbKeymap) {
        return;
    }

    // Calculate xkb modifier masks from the keymap
    xkb_mod_mask_t depressed = 0;
    if (shift) {
        xkb_mod_index_t idx = xkb_keymap_mod_get_index(m_xkbKeymap, XKB_MOD_NAME_SHIFT);
        if (idx != XKB_MOD_INVALID)
            depressed |= (1u << idx);
    }
    if (ctrl) {
        xkb_mod_index_t idx = xkb_keymap_mod_get_index(m_xkbKeymap, XKB_MOD_NAME_CTRL);
        if (idx != XKB_MOD_INVALID)
            depressed |= (1u << idx);
    }
    if (alt) {
        xkb_mod_index_t idx = xkb_keymap_mod_get_index(m_xkbKeymap, XKB_MOD_NAME_ALT);
        if (idx != XKB_MOD_INVALID)
            depressed |= (1u << idx);
    }
    if (super) {
        xkb_mod_index_t idx = xkb_keymap_mod_get_index(m_xkbKeymap, XKB_MOD_NAME_LOGO);
        if (idx != XKB_MOD_INVALID)
            depressed |= (1u << idx);
    }

    zwp_virtual_keyboard_v1_modifiers(m_virtualKeyboard, depressed, 0, 0, 0);
}

void VirtualKeyboardInjector::sendKeycode(int keycode, bool pressed)
{
    if (!m_virtualKeyboard || !m_connected) {
        return;
    }

    uint32_t time = nextTimestamp();
    uint32_t state = pressed ? WL_KEYBOARD_KEY_STATE_PRESSED : WL_KEYBOARD_KEY_STATE_RELEASED;
    zwp_virtual_keyboard_v1_key(m_virtualKeyboard, time, keycode, state);
    wl_display_flush(m_display);
}

void VirtualKeyboardInjector::sendCharacter(QChar character)
{
    if (!m_virtualKeyboard || !m_connected || !m_xkbKeymap) {
        return;
    }

    uint32_t sym = xkb_utf32_to_keysym(character.unicode());
    if (sym == XKB_KEY_NoSymbol) {
        return;
    }

    // Search for the keysym at ALL levels (not just level 0)
    // Level 0 = unshifted, Level 1 = shifted, Level 2/3 = AltGr/etc
    xkb_keycode_t min = xkb_keymap_min_keycode(m_xkbKeymap);
    xkb_keycode_t max = xkb_keymap_max_keycode(m_xkbKeymap);
    int foundCode = 0;
    int foundLevel = -1;

    for (xkb_keycode_t code = min; code <= max && foundCode == 0; code++) {
        for (int level = 0; level < 4; level++) {
            const xkb_keysym_t *syms;
            int ns = xkb_keymap_key_get_syms_by_level(m_xkbKeymap, code, 0, level, &syms);
            for (int i = 0; i < ns; i++) {
                if (syms[i] == static_cast<xkb_keysym_t>(sym)) {
                    foundCode = code - min + 1; // Convert to evdev
                    foundLevel = level;
                    break;
                }
            }
        }
    }

    if (!foundCode) {
        return;
    }

    // Send ALL Wayland events for this character in a SINGLE flush to avoid
    // the compositor processing modifier changes between flushed batches
    if (foundLevel == 1) {
        // 1. Press shift key
        zwp_virtual_keyboard_v1_key(m_virtualKeyboard, nextTimestamp(), KEY_LEFTSHIFT, WL_KEYBOARD_KEY_STATE_PRESSED);
    }

    if (foundLevel == 1) {
        // 2. Set explicit modifier mask (xkb_mod_mask_t with Shift bit)
        xkb_mod_index_t modShift = xkb_keymap_mod_get_index(m_xkbKeymap, XKB_MOD_NAME_SHIFT);
        if (modShift != XKB_MOD_INVALID) {
            zwp_virtual_keyboard_v1_modifiers(m_virtualKeyboard, (1u << modShift), 0, 0, 0);
        }
    }

    // 3. Press and release the character key
    {
        uint32_t t = nextTimestamp();
        zwp_virtual_keyboard_v1_key(m_virtualKeyboard, t, foundCode, WL_KEYBOARD_KEY_STATE_PRESSED);
    }
    {
        uint32_t t = nextTimestamp();
        zwp_virtual_keyboard_v1_key(m_virtualKeyboard, t, foundCode, WL_KEYBOARD_KEY_STATE_RELEASED);
    }

    if (foundLevel == 1) {
        // 4. Release shift key
        zwp_virtual_keyboard_v1_key(m_virtualKeyboard, nextTimestamp(), KEY_LEFTSHIFT, WL_KEYBOARD_KEY_STATE_RELEASED);
        // 5. Clear modifier mask
        zwp_virtual_keyboard_v1_modifiers(m_virtualKeyboard, 0, 0, 0, 0);
    }

    // Single flush sends ALL events at once
    wl_display_flush(m_display);
}

void VirtualKeyboardInjector::sendText(const QString &text)
{
    // Try wtype first — it handles modifiers correctly on all Wayland compositors
    static bool wtypeChecked = false;
    static bool wtypeAvailable = false;
    if (!wtypeChecked) {
        wtypeChecked = true;
        wtypeAvailable = !QProcess::execute(QStringLiteral("which"), {QStringLiteral("wtype")});
    }

    if (wtypeAvailable) {
        QProcess wtype;
        wtype.start(QStringLiteral("wtype"), {text});
        wtype.waitForFinished(1000);
        if (wtype.exitCode() == 0) {
            return; // wtype handled it
        }
    }

    if (!m_virtualKeyboard || !m_connected) {
        return;
    }

    for (const QChar &ch : text) {
        sendCharacter(ch);
    }
}

void VirtualKeyboardInjector::sendSpecialKey(int specialKey)
{
    if (!m_virtualKeyboard || !m_connected) {
        return;
    }

    int keycode = (specialKey >= 0 && specialKey < static_cast<int>(sizeof(SpecialKeysMap) / sizeof(SpecialKeysMap[0]))) ? SpecialKeysMap[specialKey] : 0;
    if (!keycode) {
        return;
    }

    uint32_t time = nextTimestamp();
    zwp_virtual_keyboard_v1_key(m_virtualKeyboard, time, keycode, WL_KEYBOARD_KEY_STATE_PRESSED);
    zwp_virtual_keyboard_v1_key(m_virtualKeyboard, time + 1, keycode, WL_KEYBOARD_KEY_STATE_RELEASED);
    wl_display_flush(m_display);
}
