/**
 * SPDX-FileCopyrightText: 2015 Martin Gräßlin <mgraesslin@kde.org>
 * SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "waylandremoteinput.h"

#include <QDebug>
#include <QSizeF>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <QDBusPendingCallWatcher>

#include <libei.h>
#include <linux/input.h>
#include <sys/mman.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon.h>

namespace
{
// Translation table to keep in sync within all the implementations
int SpecialKeysMap[] = {
    0, // Invalid
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
}

Q_GLOBAL_STATIC(RemoteDesktopSession, s_session);

class Xkb
{
public:
    Xkb()
    {
        m_xkbcontext.reset(xkb_context_new(XKB_CONTEXT_NO_FLAGS));
        m_xkbkeymap.reset(xkb_keymap_new_from_names(m_xkbcontext.get(), nullptr, XKB_KEYMAP_COMPILE_NO_FLAGS));
        m_xkbstate.reset(xkb_state_new(m_xkbkeymap.get()));
    }
    Xkb(int keymapFd, int size)
    {
        m_xkbcontext.reset(xkb_context_new(XKB_CONTEXT_NO_FLAGS));
        char *map = static_cast<char *>(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, keymapFd, 0));
        if (map != MAP_FAILED) {
            m_xkbkeymap.reset(xkb_keymap_new_from_string(m_xkbcontext.get(), map, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS));
            munmap(map, size);
        }
        close(keymapFd);
        if (!m_xkbkeymap) {
            qCWarning(KDECONNECT_PLUGIN_MOUSEPAD) << "Failed to create keymap";
            m_xkbkeymap.reset(xkb_keymap_new_from_names(m_xkbcontext.get(), nullptr, XKB_KEYMAP_COMPILE_NO_FLAGS));
        }
        m_xkbstate.reset(xkb_state_new(m_xkbkeymap.get()));
    }

    std::optional<int> keycodeFromKeysym(xkb_keysym_t keysym)
    {
        auto layout = xkb_state_serialize_layout(m_xkbstate.get(), XKB_STATE_LAYOUT_EFFECTIVE);
        const xkb_keycode_t max = xkb_keymap_max_keycode(m_xkbkeymap.get());
        for (xkb_keycode_t keycode = xkb_keymap_min_keycode(m_xkbkeymap.get()); keycode < max; keycode++) {
            uint levelCount = xkb_keymap_num_levels_for_key(m_xkbkeymap.get(), keycode, layout);
            for (uint currentLevel = 0; currentLevel < levelCount; currentLevel++) {
                const xkb_keysym_t *syms;
                uint num_syms = xkb_keymap_key_get_syms_by_level(m_xkbkeymap.get(), keycode, layout, currentLevel, &syms);
                for (uint sym = 0; sym < num_syms; sym++) {
                    if (syms[sym] == keysym) {
                        return {keycode - 8};
                    }
                }
            }
        }
        return {};
    }

private:
    template<auto D>
    using deleter = std::integral_constant<decltype(D), D>;
    std::unique_ptr<xkb_context, deleter<xkb_context_unref>> m_xkbcontext;
    std::unique_ptr<xkb_keymap, deleter<xkb_keymap_unref>> m_xkbkeymap;
    std::unique_ptr<xkb_state, deleter<xkb_state_unref>> m_xkbstate;
};

RemoteDesktopSession::RemoteDesktopSession()
    : iface(new OrgFreedesktopPortalRemoteDesktopInterface(QLatin1String("org.freedesktop.portal.Desktop"),
                                                           QLatin1String("/org/freedesktop/portal/desktop"),
                                                           QDBusConnection::sessionBus(),
                                                           this))
    , m_eiNotifier(QSocketNotifier::Read)
    , m_xkb(new Xkb)
{
    connect(&m_eiNotifier, &QSocketNotifier::activated, this, &RemoteDesktopSession::handleEiEvents);
}

void RemoteDesktopSession::createSession()
{
    if (isValid()) {
        qCDebug(KDECONNECT_PLUGIN_MOUSEPAD) << "pass, already created";
        return;
    }

    m_connecting = true;

    // create session
    const auto handleToken = QStringLiteral("kdeconnect%1").arg(QRandomGenerator::global()->generate());
    const auto sessionParameters = QVariantMap{{QLatin1String("session_handle_token"), handleToken}, {QLatin1String("handle_token"), handleToken}};
    auto sessionReply = iface->CreateSession(sessionParameters);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(sessionReply);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, sessionReply](QDBusPendingCallWatcher *self) {
        self->deleteLater();
        if (sessionReply.isError()) {
            qCWarning(KDECONNECT_PLUGIN_MOUSEPAD) << "Could not create the remote control session" << sessionReply.error();
            m_connecting = false;
            return;
        }

        bool b = QDBusConnection::sessionBus().connect(QString(),
                                                       sessionReply.value().path(),
                                                       QLatin1String("org.freedesktop.portal.Request"),
                                                       QLatin1String("Response"),
                                                       this,
                                                       SLOT(handleXdpSessionCreated(uint, QVariantMap)));
        Q_ASSERT(b);

        qCDebug(KDECONNECT_PLUGIN_MOUSEPAD) << "authenticating" << sessionReply.value().path();
    });
}

void RemoteDesktopSession::handleXdpSessionCreated(uint code, const QVariantMap &results)
{
    if (code != 0) {
        qCWarning(KDECONNECT_PLUGIN_MOUSEPAD) << "Failed to create session with code" << code << results;
        return;
    }

    m_connecting = false;
    m_xdpPath = QDBusObjectPath(results.value(QLatin1String("session_handle")).toString());
    QVariantMap startParameters = {
        {QLatin1String("handle_token"), QStringLiteral("kdeconnect%1").arg(QRandomGenerator::global()->generate())},
        {QStringLiteral("types"), QVariant::fromValue<uint>(7)}, // request all (KeyBoard, Pointer, TouchScreen)
        {QLatin1String("persist_mode"), QVariant::fromValue<uint>(2)}, // Persist permission until explicitly revoked by user
    };

    KConfigGroup stateConfig = KSharedConfig::openStateConfig()->group(QStringLiteral("mousepad"));
    QString restoreToken = stateConfig.readEntry(QStringLiteral("RestoreToken"), QString());
    if (restoreToken.length() > 0) {
        startParameters[QLatin1String("restore_token")] = restoreToken;
    }

    QDBusConnection::sessionBus().connect(QString(),
                                          m_xdpPath.path(),
                                          QLatin1String("org.freedesktop.portal.Session"),
                                          QLatin1String("Closed"),
                                          this,
                                          SLOT(handleXdpSessionFinished(uint, QVariantMap)));

    auto reply = iface->SelectDevices(m_xdpPath, startParameters);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, reply](QDBusPendingCallWatcher *self) {
        self->deleteLater();
        if (reply.isError()) {
            qCWarning(KDECONNECT_PLUGIN_MOUSEPAD) << "Could not start the remote control session" << reply.error();
            m_connecting = false;
            return;
        }

        bool b = QDBusConnection::sessionBus().connect(QString(),
                                                       reply.value().path(),
                                                       QLatin1String("org.freedesktop.portal.Request"),
                                                       QLatin1String("Response"),
                                                       this,
                                                       SLOT(handleXdpSessionConfigured(uint, QVariantMap)));
        Q_ASSERT(b);
        qCDebug(KDECONNECT_PLUGIN_MOUSEPAD) << "configuring" << reply.value().path();
    });
}

void RemoteDesktopSession::handleXdpSessionConfigured(uint code, const QVariantMap &results)
{
    if (code != 0) {
        qCWarning(KDECONNECT_PLUGIN_MOUSEPAD) << "Failed to configure session with code" << code << results;
        m_connecting = false;
        return;
    }
    const QVariantMap startParameters = {
        {QLatin1String("handle_token"), QStringLiteral("kdeconnect%1").arg(QRandomGenerator::global()->generate())},
    };
    auto reply = iface->Start(m_xdpPath, {}, startParameters);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, reply](QDBusPendingCallWatcher *self) {
        self->deleteLater();
        if (reply.isError()) {
            qCWarning(KDECONNECT_PLUGIN_MOUSEPAD) << "Could not start the remote control session" << reply.error();
            m_connecting = false;
            return;
        }

        bool b = QDBusConnection::sessionBus().connect(QString(),
                                                       reply.value().path(),
                                                       QLatin1String("org.freedesktop.portal.Request"),
                                                       QLatin1String("Response"),
                                                       this,
                                                       SLOT(handleXdpSessionStarted(uint, QVariantMap)));
        Q_ASSERT(b);
        qCDebug(KDECONNECT_PLUGIN_MOUSEPAD) << "starting" << reply.value().path();
    });
}

void RemoteDesktopSession::handleXdpSessionStarted(uint code, const QVariantMap &results)
{
    Q_UNUSED(code);

    KConfigGroup stateConfig = KSharedConfig::openStateConfig()->group(QStringLiteral("mousepad"));
    stateConfig.writeEntry(QStringLiteral("RestoreToken"), results[QStringLiteral("restore_token")].toString());
    auto call = iface->ConnectToEIS(m_xdpPath, {});
    connect(new QDBusPendingCallWatcher(call), &QDBusPendingCallWatcher::finished, [this](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        QDBusReply<QDBusUnixFileDescriptor> reply = *watcher;
        if (!reply.isValid()) {
            qCWarning(KDECONNECT_PLUGIN_MOUSEPAD) << "Error connecting to eis" << reply.error();
            return;
        }
        connectToEi(reply.value().takeFileDescriptor());
    });
}

void RemoteDesktopSession::connectToEi(int fd)
{
    m_eiNotifier.setSocket(fd);
    m_eiNotifier.setEnabled(true);
    m_ei = ei_new_sender(this);
    ei_log_set_handler(m_ei, nullptr);
    ei_setup_backend_fd(m_ei, fd);
}

void RemoteDesktopSession::handleEiEvents()
{
    ei_dispatch(m_ei);
    while (auto event = ei_get_event(m_ei)) {
        const auto type = ei_event_get_type(event);
        switch (type) {
        case EI_EVENT_CONNECT:
            qCDebug(KDECONNECT_PLUGIN_MOUSEPAD) << "Connected to ei";
            break;
        case EI_EVENT_DISCONNECT:
            qCWarning(KDECONNECT_PLUGIN_MOUSEPAD) << "Disconnected from ei";
            break;
        case EI_EVENT_SEAT_ADDED: {
            auto seat = ei_event_get_seat(event);
            ei_seat_bind_capabilities(seat,
                                      EI_DEVICE_CAP_KEYBOARD,
                                      EI_DEVICE_CAP_POINTER,
                                      EI_DEVICE_CAP_POINTER_ABSOLUTE,
                                      EI_DEVICE_CAP_BUTTON,
                                      EI_DEVICE_CAP_SCROLL,
                                      nullptr);
            break;
        }
        case EI_EVENT_SEAT_REMOVED:
            break;
        case EI_EVENT_DEVICE_ADDED: {
            auto device = ei_event_get_device(event);
            if (ei_device_has_capability(device, EI_DEVICE_CAP_KEYBOARD) && !m_keyboard) {
                auto keymap = ei_device_keyboard_get_keymap(device);
                if (ei_keymap_get_type(keymap) != EI_KEYMAP_TYPE_XKB) {
                    break;
                }
                m_xkb.reset(new Xkb(ei_keymap_get_fd(keymap), ei_keymap_get_size(keymap)));
                m_keyboard = device;
            }
            if (ei_device_has_capability(device, EI_DEVICE_CAP_POINTER) && !m_pointer) {
                m_pointer = device;
            }
            if (ei_device_has_capability(device, EI_DEVICE_CAP_POINTER_ABSOLUTE) && !m_absolutePointer) {
                m_absolutePointer = device;
            }
        } break;
        case EI_EVENT_DEVICE_REMOVED: {
            auto device = ei_event_get_device(event);
            if (device == m_keyboard) {
                m_keyboard = nullptr;
            }
            if (device == m_pointer) {
                m_pointer = nullptr;
            }
            if (device == m_absolutePointer) {
                m_absolutePointer = nullptr;
            }
            break;
        }
        case EI_EVENT_DEVICE_PAUSED:
            break;
        case EI_EVENT_DEVICE_RESUMED:
            ei_device_start_emulating(ei_event_get_device(event), 0);
            break;

        case EI_EVENT_FRAME:
        case EI_EVENT_POINTER_MOTION:
        case EI_EVENT_POINTER_MOTION_ABSOLUTE:
        case EI_EVENT_BUTTON_BUTTON:
        case EI_EVENT_SCROLL_DELTA:
        case EI_EVENT_SCROLL_DISCRETE:
        case EI_EVENT_SCROLL_STOP:
        case EI_EVENT_SCROLL_CANCEL:
        case EI_EVENT_KEYBOARD_MODIFIERS:
        case EI_EVENT_KEYBOARD_KEY:
        case EI_EVENT_DEVICE_START_EMULATING:
        case EI_EVENT_DEVICE_STOP_EMULATING:
        case EI_EVENT_TOUCH_DOWN:
        case EI_EVENT_TOUCH_MOTION:
        case EI_EVENT_TOUCH_UP:
            qCDebug(KDECONNECT_PLUGIN_MOUSEPAD) << "Unexpected event of type" << ei_event_get_type(event);
            break;
        }
        ei_event_unref(event);
    }
}

void RemoteDesktopSession::handleXdpSessionFinished(uint /*code*/, const QVariantMap & /*results*/)
{
    m_xdpPath = {};
    m_eiNotifier.setEnabled(false);
    ei_unref(m_ei);
    m_pointer = nullptr;
    m_keyboard = nullptr;
    m_absolutePointer = nullptr;
}

void RemoteDesktopSession::pointerButton(int button, bool down)
{
    if (m_ei && m_pointer) {
        ei_device_button_button(m_pointer, button, down);
        ei_device_frame(m_pointer, ei_now(m_ei));
    } else {
        iface->NotifyPointerButton(m_xdpPath, {}, BTN_LEFT, down);
    }
}

void RemoteDesktopSession::pointerAxis(double dx, double dy)
{
    if (m_ei && m_pointer) {
        // Qt/Kdeconnect use inverted vertical scroll direction compared to libei
        ei_device_scroll_delta(m_pointer, dx, dy * -1);
        ei_device_frame(m_pointer, ei_now(m_ei));
    } else {
        iface->NotifyPointerAxis(m_xdpPath, {}, dx, dy);
    }
}

void RemoteDesktopSession::pointerMotion(double dx, double dy)
{
    if (m_ei && m_pointer) {
        ei_device_pointer_motion(m_pointer, dx, dy);
        ei_device_frame(m_pointer, ei_now(m_ei));
    } else {
        iface->NotifyPointerMotion(m_xdpPath, {}, dx, dy);
    }
}

void RemoteDesktopSession::pointerMotionAbsolute(double x, double y)
{
    if (m_ei && m_absolutePointer) {
        qDebug() << x << y;
        ei_device_pointer_motion_absolute(m_absolutePointer, x, y);
        ei_device_frame(m_absolutePointer, ei_now(m_ei));
    } else {
        iface->NotifyPointerMotionAbsolute(m_xdpPath, {}, 0, x, y);
    }
}

void RemoteDesktopSession::keyboardKeycode(int key, bool press)
{
    if (m_ei && m_keyboard) {
        ei_device_keyboard_key(m_keyboard, key, press);
        ei_device_frame(m_keyboard, ei_now(m_ei));
    } else {
        iface->NotifyKeyboardKeycode(m_xdpPath, {}, key, press);
    }
}

void RemoteDesktopSession::keyboardKeysym(int sym, bool press)
{
    if (m_ei && m_keyboard) {
        if (auto code = m_xkb->keycodeFromKeysym(sym)) {
            ei_device_keyboard_key(m_keyboard, *code, press);
            ei_device_frame(m_keyboard, ei_now(m_ei));
        } else {
            qCWarning(KDECONNECT_PLUGIN_MOUSEPAD) << "failed to convert keysym" << sym;
        }
    } else {
        iface->NotifyKeyboardKeysym(m_xdpPath, {}, sym, press).waitForFinished();
    }
}

WaylandRemoteInput::WaylandRemoteInput(QObject *parent)
    : AbstractRemoteInput(parent)
{
}

bool WaylandRemoteInput::handlePacket(const NetworkPacket &np)
{
    if (!s_session->isValid()) {
        qCWarning(KDECONNECT_PLUGIN_MOUSEPAD) << "Unable to handle remote input. RemoteDesktop portal not authenticated";
        s_session->createSession();
        return false;
    }

    const float dx = np.get<float>(QStringLiteral("dx"), 0);
    const float dy = np.get<float>(QStringLiteral("dy"), 0);
    const float x = np.get<float>(QStringLiteral("x"), 0);
    const float y = np.get<float>(QStringLiteral("y"), 0);

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
            s_session->pointerButton(BTN_LEFT, true);
            s_session->pointerButton(BTN_LEFT, false);
        } else if (isDoubleClick) {
            s_session->pointerButton(BTN_LEFT, true);
            s_session->pointerButton(BTN_LEFT, false);
            s_session->pointerButton(BTN_LEFT, true);
            s_session->pointerButton(BTN_LEFT, false);
        } else if (isMiddleClick) {
            s_session->pointerButton(BTN_MIDDLE, true);
            s_session->pointerButton(BTN_MIDDLE, false);
        } else if (isRightClick) {
            s_session->pointerButton(BTN_RIGHT, true);
            s_session->pointerButton(BTN_RIGHT, false);
        } else if (isSingleHold) {
            // For drag'n drop
            s_session->pointerButton(BTN_LEFT, true);
        } else if (isSingleRelease) {
            s_session->pointerButton(BTN_LEFT, false);
        } else if (isScroll) {
            s_session->pointerAxis(dx, dy);
        } else if (specialKey || !key.isEmpty()) {
            bool ctrl = np.get<bool>(QStringLiteral("ctrl"), false);
            bool alt = np.get<bool>(QStringLiteral("alt"), false);
            bool shift = np.get<bool>(QStringLiteral("shift"), false);
            bool super = np.get<bool>(QStringLiteral("super"), false);

            if (ctrl)
                s_session->keyboardKeycode(KEY_LEFTCTRL, true);
            if (alt)
                s_session->keyboardKeycode(KEY_LEFTALT, true);
            if (shift)
                s_session->keyboardKeycode(KEY_LEFTSHIFT, true);
            if (super)
                s_session->keyboardKeycode(KEY_LEFTMETA, true);

            if (specialKey) {
                s_session->keyboardKeycode(SpecialKeysMap[specialKey], true);
                s_session->keyboardKeycode(SpecialKeysMap[specialKey], false);
            } else if (!key.isEmpty()) {
                for (const QChar character : key) {
                    const auto keysym = xkb_utf32_to_keysym(character.unicode());
                    if (keysym != XKB_KEY_NoSymbol) {
                        s_session->keyboardKeysym(keysym, true);
                        s_session->keyboardKeysym(keysym, false);
                    } else {
                        qCDebug(KDECONNECT_PLUGIN_MOUSEPAD) << "Cannot send character" << character;
                    }
                }
            }

            if (ctrl)
                s_session->keyboardKeycode(KEY_LEFTCTRL, false);
            if (alt)
                s_session->keyboardKeycode(KEY_LEFTALT, false);
            if (shift)
                s_session->keyboardKeycode(KEY_LEFTSHIFT, false);
            if (super)
                s_session->keyboardKeycode(KEY_LEFTMETA, false);
        }
    } else { // Is a mouse move event
        if (dx || dy)
            s_session->pointerMotion(dx, dy);
        else if ((np.has(QStringLiteral("x")) || np.has(QStringLiteral("y"))))
            s_session->pointerMotionAbsolute(x, y);
    }
    return true;
}

#include "moc_waylandremoteinput.cpp"
