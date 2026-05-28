/**
 * SPDX-FileCopyrightText: 2025 James Padgett <snotacusnexus@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>
#include <QString>

struct wl_display;
struct wl_registry;
struct wl_seat;
struct zwp_virtual_keyboard_manager_v1;
struct zwp_virtual_keyboard_v1;
struct xkb_context;
struct xkb_keymap;

/**
 * @brief Injects keyboard input through the wlr-virtual-keyboard protocol.
 *
 * Opens a dedicated Wayland connection, binds to the virtual keyboard
 * manager, and sends key events directly to the compositor.
 *
 * This bypasses the RemoteDesktop portal, enabling keyboard input
 * on wlroots-based compositors (Hyprland, Sway, etc.) that don't
 * implement the portal's RemoteDesktop interface.
 */
class VirtualKeyboardInjector : public QObject
{
    Q_OBJECT

public:
    explicit VirtualKeyboardInjector(QObject *parent = nullptr);
    ~VirtualKeyboardInjector() override;

    /**
     * Checks if the compositor supports zwp_virtual_keyboard_manager_v1.
     */
    static bool isAvailable();

    /**
     * Inject a keycode event (press or release).
     * @param keycode evdev keycode from linux/input.h
     * @param pressed true = press, false = release
     */
    void sendKeycode(int keycode, bool pressed);

    /**
     * Inject a character as a key press+release.
     * Converts the unicode character to a keysym, then to a keycode.
     */
    void sendCharacter(QChar character);

    /**
     * Inject a text string as individual key press+release events.
     * Each character is typed separately.
     */
    void sendText(const QString &text);

    /**
     * Inject a special key (from the SpecialKeysMap table).
     */
    void sendSpecialKey(int specialKey);

    /**
     * Set the keyboard modifier state explicitly.
     * Sends a modifiers request to the compositor with the specified modifier mask.
     */
    void setModifiers(bool shift, bool ctrl, bool alt, bool super);

private:
    bool connectToCompositor();
    void disconnectFromCompositor();
    uint32_t keycodeFromKeysym(uint32_t sym);
    uint32_t nextTimestamp();
    void ensureKeymap();

    static void sRegistryGlobal(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);

    struct wl_display *m_display = nullptr;
    struct wl_registry *m_registry = nullptr;
    struct wl_seat *m_seat = nullptr;
    struct zwp_virtual_keyboard_manager_v1 *m_keyboardManager = nullptr;
    struct zwp_virtual_keyboard_v1 *m_virtualKeyboard = nullptr;

    struct xkb_context *m_xkbCtx = nullptr;
    struct xkb_keymap *m_xkbKeymap = nullptr;

    uint32_t m_lastTimestamp = 0;
    bool m_connected = false;
};
