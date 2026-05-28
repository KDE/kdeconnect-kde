/**
 * SPDX-FileCopyrightText: 2025 James Padgett <snotacusnexus@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "abstractremoteinput.h"

#include <QSocketNotifier>

struct wl_display;
struct wl_registry;
struct wl_seat;
struct zwlr_virtual_pointer_manager_v1;
struct zwlr_virtual_pointer_v1;
struct zwp_virtual_keyboard_manager_v1;
struct zwp_virtual_keyboard_v1;

class Xkb;

/**
 * @brief Remote input backend for wlroots-based Wayland compositors (Hyprland, Sway, etc.)
 *
 * Uses the wlr-virtual-pointer and wlr-virtual-keyboard protocols directly,
 * bypassing the RemoteDesktop portal (which isn't implemented by all compositors).
 *
 * This allows KDE Connect's mousepad to work on Hyprland and other wlroots
 * compositors without portal support for remote input.
 */
class HyprlandRemoteInput : public AbstractRemoteInput
{
    Q_OBJECT

public:
    explicit HyprlandRemoteInput(QObject *parent);
    ~HyprlandRemoteInput() override;

    bool handlePacket(const NetworkPacket &np) override;
    bool hasKeyboardSupport() override;

    /**
     * Checks if the current Wayland compositor supports the protocols
     * required by this backend (wlr-virtual-pointer).
     */
    static bool isAvailable();

private:
    bool connectToCompositor();
    void disconnectFromCompositor();
    void ensurePointer();
    void ensureKeyboard();

    void handlePointerMotion(double dx, double dy);
    void handlePointerMotionAbsolute(double x, double y);
    void handlePointerButton(int button, bool pressed);
    void handlePointerAxis(double dx, double dy);
    void handlePointerFrame();

    void handleKeyboardKeycode(int key, bool pressed);
    void handleKeyboardKeysym(uint32_t sym, bool pressed);
    void handleKeyboardModifiers(bool ctrl, bool alt, bool shift, bool super);

    static void sRegistryGlobal(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
    static void sRegistryGlobalRemove(void *data, struct wl_registry *registry, uint32_t name);

    // Wayland protocol objects
    struct wl_display *m_display = nullptr;
    struct wl_registry *m_registry = nullptr;
    struct wl_seat *m_seat = nullptr;
    struct zwlr_virtual_pointer_manager_v1 *m_pointerManager = nullptr;
    struct zwlr_virtual_pointer_v1 *m_virtualPointer = nullptr;
    struct zwp_virtual_keyboard_manager_v1 *m_keyboardManager = nullptr;
    struct zwp_virtual_keyboard_v1 *m_virtualKeyboard = nullptr;

    // Keymap
    std::unique_ptr<Xkb> m_xkb;

    // Socket notifier for Wayland events
    QSocketNotifier *m_notifier = nullptr;

    bool m_ready = false;
};
