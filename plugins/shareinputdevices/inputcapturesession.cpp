/**
 * SPDX-FileCopyrightText: 2024 David Redondo <kde@david-redondo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "inputcapturesession.h"
#include "plugin_shareinputdevices_debug.h"

#include "generated/systeminterfaces/inputcapture.h"
#include "generated/systeminterfaces/request.h"
#include "generated/systeminterfaces/session.h"

#include <QRandomGenerator>
#include <private/qxkbcommon_p.h>

#include <libei.h>
#include <xkbcommon/xkbcommon.h>

#include <sys/mman.h>
#include <unistd.h>

#include <ranges>

using namespace Qt::StringLiterals;

static QString portalName()
{
    return u"org.freedesktop.portal.Desktop"_s;
}

static QString portalPath()
{
    return u"/org/freedesktop/portal/desktop"_s;
};

static QString requestPath(const QString &token)
{
    static QString senderString = QDBusConnection::sessionBus().baseService().remove(0, 1).replace(u'.', u'_');
    return u"%1/request/%2/%3"_s.arg(portalPath()).arg(senderString).arg(token);
};

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
            qCWarning(KDECONNECT_PLUGIN_SHAREINPUTDEVICES) << "Failed to create keymap";
            m_xkbkeymap.reset(xkb_keymap_new_from_names(m_xkbcontext.get(), nullptr, XKB_KEYMAP_COMPILE_NO_FLAGS));
        }
        m_xkbstate.reset(xkb_state_new(m_xkbkeymap.get()));
    }

    void updateModifiers(uint32_t depressed, uint32_t latched, uint32_t locked, uint32_t group)
    {
        xkb_state_update_mask(m_xkbstate.get(), depressed, latched, locked, 0, 0, group);
    }

    void updateKey(uint32_t key, bool pressed)
    {
        xkb_state_update_key(m_xkbstate.get(), key, pressed ? XKB_KEY_DOWN : XKB_KEY_UP);
    }

    xkb_state *currentState() const
    {
        return m_xkbstate.get();
    }

private:
    template<auto D>
    using deleter = std::integral_constant<decltype(D), D>;
    std::unique_ptr<xkb_context, deleter<xkb_context_unref>> m_xkbcontext;
    std::unique_ptr<xkb_keymap, deleter<xkb_keymap_unref>> m_xkbkeymap;
    std::unique_ptr<xkb_state, deleter<xkb_state_unref>> m_xkbstate;
};

InputCaptureSession::InputCaptureSession(QObject *parent)
    : QObject(parent)
    , m_xkb(new Xkb)
    , m_inputCapturePortal(new OrgFreedesktopPortalInputCaptureInterface(portalName(), portalPath(), QDBusConnection::sessionBus(), this))

{
    connect(m_inputCapturePortal, &OrgFreedesktopPortalInputCaptureInterface::Disabled, this, &InputCaptureSession::disabled);
    connect(m_inputCapturePortal, &OrgFreedesktopPortalInputCaptureInterface::Activated, this, &InputCaptureSession::activated);
    connect(m_inputCapturePortal, &OrgFreedesktopPortalInputCaptureInterface::Deactivated, this, &InputCaptureSession::deactivated);
    connect(m_inputCapturePortal, &OrgFreedesktopPortalInputCaptureInterface::ZonesChanged, this, &InputCaptureSession::zonesChanged);

    const QString token = u"kdeconnect_shareinputdevices%1"_s.arg(QRandomGenerator::global()->generate());
    auto request = new OrgFreedesktopPortalRequestInterface(portalName(), requestPath(token), QDBusConnection::sessionBus(), this);
    connect(request, &OrgFreedesktopPortalRequestInterface::Response, request, &QObject::deleteLater);
    connect(request, &OrgFreedesktopPortalRequestInterface::Response, this, &InputCaptureSession::sessionCreated);
    auto call = m_inputCapturePortal->CreateSession(QString(), {{u"handle_token"_s, token}, {u"session_handle_token"_s, token}, {u"capabilities"_s, 3u}});
    connect(new QDBusPendingCallWatcher(call, this), &QDBusPendingCallWatcher::finished, request, [request](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        if (watcher->isError()) {
            qCWarning(KDECONNECT_PLUGIN_SHAREINPUTDEVICES) << "Error creating input capture session" << watcher->error();
            request->deleteLater();
        }
    });
}

InputCaptureSession::~InputCaptureSession()
{
    if (m_session) {
        m_session->Close();
    }
    if (m_ei) {
        ei_unref(m_ei);
    }
}

void InputCaptureSession::sessionCreated(uint response, const QVariantMap &results)
{
    if (response != 0) {
        qCWarning(KDECONNECT_PLUGIN_SHAREINPUTDEVICES) << "Couldn't create input capture session";
        return;
    }
    m_session = std::make_unique<OrgFreedesktopPortalSessionInterface>(portalName(),
                                                                       results[u"session_handle"_s].value<QDBusObjectPath>().path(),
                                                                       QDBusConnection::sessionBus());
    connect(m_session.get(), &OrgFreedesktopPortalSessionInterface::Closed, this, &InputCaptureSession::sessionClosed);

    auto call = m_inputCapturePortal->ConnectToEIS(QDBusObjectPath(m_session->path()), {});
    connect(new QDBusPendingCallWatcher(call, this), &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        QDBusReply<QDBusUnixFileDescriptor> reply = *watcher;
        if (!reply.isValid()) {
            qCWarning(KDECONNECT_PLUGIN_SHAREINPUTDEVICES) << "Error getting eis fd" << watcher->error();
            return;
        }
        qCDebug(KDECONNECT_PLUGIN_SHAREINPUTDEVICES) << "Received ei fd" << reply.value().fileDescriptor();
        setupEi(reply.value().takeFileDescriptor());
    });
    getZones();
}

void InputCaptureSession::getZones()
{
    const QString token = u"kdeconnect_shareinputdevices%1"_s.arg(QRandomGenerator::global()->generate());
    auto request = new OrgFreedesktopPortalRequestInterface(portalName(), requestPath(token), QDBusConnection::sessionBus(), this);
    connect(request, &OrgFreedesktopPortalRequestInterface::Response, request, &QObject::deleteLater);
    connect(request, &OrgFreedesktopPortalRequestInterface::Response, this, &InputCaptureSession::zonesReceived);
    auto call = m_inputCapturePortal->GetZones(QDBusObjectPath(m_session->path()), {{u"handle_token"_s, token}});
    connect(new QDBusPendingCallWatcher(call, this), &QDBusPendingCallWatcher::finished, request, [request](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        if (watcher->isError()) {
            qCWarning(KDECONNECT_PLUGIN_SHAREINPUTDEVICES) << "Error getting zones" << watcher->error();
            request->deleteLater();
        }
    });
}

void InputCaptureSession::zonesReceived(uint response, const QVariantMap &results)
{
    if (response != 0) {
        qCWarning(KDECONNECT_PLUGIN_SHAREINPUTDEVICES) << "Couldn't create input capture session";
        return;
    }
    m_currentZoneSet = results[u"zone_set"_s].toUInt();
    m_currentZones.clear();
    const QDBusArgument zoneArgument = results[u"zones"_s].value<QDBusArgument>();
    zoneArgument.beginArray();
    while (!zoneArgument.atEnd()) {
        int width;
        int height;
        uint x;
        uint y;
        zoneArgument.beginStructure();
        zoneArgument >> width >> height >> x >> y;
        zoneArgument.endStructure();
        m_currentZones.push_back(QRect(x, y, width, height));
    }
    zoneArgument.endArray();

    setUpBarrier();
}

void InputCaptureSession::setUpBarrier()
{
    // Find the left/right/bottom/top-most screen
    if (m_barrierEdge == Qt::LeftEdge) {
        std::stable_sort(m_currentZones.begin(), m_currentZones.end(), [](const QRect &lhs, const QRect &rhs) {
            return lhs.x() < rhs.x();
        });
        const auto &zone = m_currentZones.front();
        // Deliberate QRect::bottom usage, on a 1920x1080 screen this needs to be 1079
        m_barrier = {zone.x(), zone.y(), zone.x(), zone.bottom()};
    } else if (m_barrierEdge == Qt::RightEdge) {
        std::stable_sort(m_currentZones.begin(), m_currentZones.end(), [](const QRect &lhs, const QRect &rhs) {
            return lhs.x() + lhs.width() > rhs.x() + rhs.width();
        });
        const auto &zone = m_currentZones.front();
        m_barrier = {zone.x() + zone.width(), zone.y(), zone.x() + zone.width(), zone.bottom()};
    } else if (m_barrierEdge == Qt::TopEdge) {
        std::stable_sort(m_currentZones.begin(), m_currentZones.end(), [](const QRect &lhs, const QRect &rhs) {
            return lhs.y() < rhs.y();
        });
        const auto &zone = m_currentZones.front();
        // Same here with QRect::right
        m_barrier = {zone.x(), zone.y(), zone.right(), zone.y()};
    } else {
        std::stable_sort(m_currentZones.begin(), m_currentZones.end(), [](const QRect &lhs, const QRect &rhs) {
            return lhs.y() + lhs.height() > rhs.y() + rhs.height();
        });
        const auto &zone = m_currentZones.front();
        m_barrier = {zone.x(), zone.y() + zone.height(), zone.right(), zone.y() + zone.height()};
    }

    const QString token = u"kdeconnect_shareinputdevices%1"_s.arg(QRandomGenerator::global()->generate());
    auto request = new OrgFreedesktopPortalRequestInterface(portalName(), requestPath(token), QDBusConnection::sessionBus(), this);
    connect(request, &OrgFreedesktopPortalRequestInterface::Response, request, &QObject::deleteLater);
    connect(request, &OrgFreedesktopPortalRequestInterface::Response, this, &InputCaptureSession::barriersSet);
    auto call = m_inputCapturePortal->SetPointerBarriers(
        QDBusObjectPath(m_session->path()),
        {{u"handle_token"_s, token}},
        {{{u"barrier_id"_s, 1}, {u"position"_s, QVariant::fromValue(QList<int>{m_barrier.x1(), m_barrier.y1(), m_barrier.x2(), m_barrier.y2()})}}},
        m_currentZoneSet);
    connect(new QDBusPendingCallWatcher(call, this), &QDBusPendingCallWatcher::finished, request, [request](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        if (watcher->isError()) {
            qCWarning(KDECONNECT_PLUGIN_SHAREINPUTDEVICES) << "Error setting barriers" << watcher->error();
            request->deleteLater();
        }
    });
}

void InputCaptureSession::barriersSet(uint response, const QVariantMap &results)
{
    if (response != 0) {
        qCWarning(KDECONNECT_PLUGIN_SHAREINPUTDEVICES) << "Couldn't set barriers";
        return;
    }
    auto failedBarriers = qdbus_cast<QList<uint>>(results[u"failed_barriers"_s].value<QDBusArgument>());
    if (!failedBarriers.empty()) {
        qCInfo(KDECONNECT_PLUGIN_SHAREINPUTDEVICES) << "Failed barriers" << failedBarriers;
    }
    enable();
}

void InputCaptureSession::enable()
{
    auto call = m_inputCapturePortal->Enable(QDBusObjectPath(m_session->path()), {});
    connect(new QDBusPendingCallWatcher(call, this), &QDBusPendingCallWatcher::finished, this, [](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        if (watcher->isError()) {
            qCWarning(KDECONNECT_PLUGIN_SHAREINPUTDEVICES) << "Failed enabling input capture session" << watcher->error();
        }
    });
}

void InputCaptureSession::setBarrierEdge(Qt::Edge edge)
{
    if (edge != m_barrierEdge) {
        m_barrierEdge = edge;
        if (!m_currentZones.empty()) {
            setUpBarrier();
        }
    }
}

void InputCaptureSession::release(const QPointF &position)
{
    qDebug() << "releasing with" << position;
    m_inputCapturePortal->Release(QDBusObjectPath(m_session->path()), {{u"cursor_position"_s, position}});
}

void InputCaptureSession::sessionClosed()
{
    qCCritical(KDECONNECT_PLUGIN_SHAREINPUTDEVICES()) << "input capture session was closed";
    m_session.reset();
    m_eiNotifier.reset();
}

void InputCaptureSession::activated(const QDBusObjectPath &sessionHandle, const QVariantMap &options)
{
    if (!m_session || sessionHandle.path() != m_session->path()) {
        return;
    }
    m_currentActivationId = options[u"activation_id"_s].toUInt();
    // uint barrier_id = options[u"barrier_id"].toUInt();
    auto cursorPosition = qdbus_cast<QPointF>(options[u"cursor_position"_s].value<QDBusArgument>());
    Q_EMIT started(m_barrier, cursorPosition - m_barrier.p1());
    for (const auto &event : queuedEiEvents) {
        handleEiEvent(event);
    }
    queuedEiEvents.clear();
}

void InputCaptureSession::deactivated(const QDBusObjectPath &sessionHandle, const QVariantMap &options)
{
    if (!m_session || sessionHandle.path() != m_session->path()) {
        return;
    }
    auto deactivatedId = options[u"activation_id"_s].toUInt();
    Q_UNUSED(deactivatedId)
}

void InputCaptureSession::disabled(const QDBusObjectPath &sessionHandle, const QVariantMap &options)
{
    if (!m_session || sessionHandle.path() != m_session->path()) {
        return;
    }
    auto disabledId = options[u"activation_id"_s].toUInt();
    Q_UNUSED(disabledId)
}

void InputCaptureSession::zonesChanged(const QDBusObjectPath &sessionHandle, const QVariantMap &options)
{
    if (!m_session || sessionHandle.path() != m_session->path()) {
        return;
    }
    if (options[u"zone_set"_s].toUInt() >= m_currentZoneSet) {
        getZones();
    }
}

void InputCaptureSession::setupEi(int fd)
{
    m_ei = ei_new_receiver(nullptr);
    ei_setup_backend_fd(m_ei, fd);
    m_eiNotifier = std::make_unique<QSocketNotifier>(fd, QSocketNotifier::Read);
    connect(m_eiNotifier.get(), &QSocketNotifier::activated, this, [this] {
        ei_dispatch(m_ei);
        while (auto event = ei_get_event(m_ei)) {
            handleEiEvent(event);
        }
    });
}

void InputCaptureSession::handleEiEvent(ei_event *event)
{
    const auto type = ei_event_get_type(event);
    constexpr std::array inputEvents = {
        EI_EVENT_FRAME,
        EI_EVENT_POINTER_MOTION,
        EI_EVENT_POINTER_MOTION_ABSOLUTE,
        EI_EVENT_BUTTON_BUTTON,
        EI_EVENT_SCROLL_DELTA,
        EI_EVENT_SCROLL_DISCRETE,
        EI_EVENT_SCROLL_STOP,
        EI_EVENT_SCROLL_CANCEL,
        EI_EVENT_KEYBOARD_MODIFIERS,
        EI_EVENT_KEYBOARD_KEY,
        EI_EVENT_TOUCH_DOWN,
        EI_EVENT_TOUCH_MOTION,
        EI_EVENT_TOUCH_UP,
    };
    if (m_currentEisSequence > m_currentActivationId && std::find(inputEvents.begin(), inputEvents.end(), type) != inputEvents.end()) {
        // Wait until DBus activated signal to have correct start position
        queuedEiEvents.push_back(event);
        return;
    }

    switch (type) {
    case EI_EVENT_CONNECT:
        qCDebug(KDECONNECT_PLUGIN_SHAREINPUTDEVICES) << "Connected to ei";
        break;
    case EI_EVENT_DISCONNECT:
        qCWarning(KDECONNECT_PLUGIN_SHAREINPUTDEVICES) << "Disconnected from ei";
        break;
    case EI_EVENT_SEAT_ADDED: {
        auto seat = ei_event_get_seat(event);
        ei_seat_bind_capabilities(seat, EI_DEVICE_CAP_KEYBOARD, EI_DEVICE_CAP_POINTER, EI_DEVICE_CAP_BUTTON, EI_DEVICE_CAP_SCROLL, nullptr);
        break;
    }
    case EI_EVENT_SEAT_REMOVED:
        break;
    case EI_EVENT_DEVICE_ADDED: {
        auto device = ei_event_get_device(event);
        if (ei_device_has_capability(device, EI_DEVICE_CAP_KEYBOARD)) {
            auto keymap = ei_device_keyboard_get_keymap(device);
            if (ei_keymap_get_type(keymap) != EI_KEYMAP_TYPE_XKB) {
                break;
            }
            m_xkb.reset(new Xkb(ei_keymap_get_fd(keymap), ei_keymap_get_size(keymap)));
        }
    } break;
    case EI_EVENT_DEVICE_REMOVED:
        break;
    case EI_EVENT_DEVICE_START_EMULATING:
        m_currentEisSequence = ei_event_emulating_get_sequence(event);
        break;
    case EI_EVENT_DEVICE_STOP_EMULATING:
        break;
    case EI_EVENT_FRAME:
        break;
    case EI_EVENT_POINTER_MOTION:
        if (m_currentEisSequence < m_currentActivationId) {
            queuedEiEvents.push_back(event);
        }
        Q_EMIT mouseMove(ei_event_pointer_get_dx(event), ei_event_pointer_get_dy(event));
        break;
    case EI_EVENT_POINTER_MOTION_ABSOLUTE:
        break;
    case EI_EVENT_BUTTON_BUTTON:
        Q_EMIT mouseButton(ei_event_button_get_button(event), ei_event_button_get_is_press(event));
        break;
    case EI_EVENT_SCROLL_DELTA:
        Q_EMIT scrollDelta(ei_event_scroll_get_dx(event), ei_event_scroll_get_dy(event));
        break;
    case EI_EVENT_SCROLL_DISCRETE:
        Q_EMIT scrollDiscrete(ei_event_scroll_get_discrete_dx(event), ei_event_scroll_get_discrete_dy(event));
        break;
    case EI_EVENT_SCROLL_STOP:
        break;
    case EI_EVENT_SCROLL_CANCEL:
        break;
    case EI_EVENT_KEYBOARD_MODIFIERS:
        m_xkb->updateModifiers(ei_event_keyboard_get_xkb_mods_depressed(event),
                               ei_event_keyboard_get_xkb_mods_latched(event),
                               ei_event_keyboard_get_xkb_mods_locked(event),
                               ei_event_keyboard_get_xkb_group(event));
        break;
    case EI_EVENT_KEYBOARD_KEY: {
        auto xkbKey = ei_event_keyboard_get_key(event) + 8;
        m_xkb->updateKey(xkbKey, ei_event_keyboard_get_key_is_press(event));
        // mousepad plugin does press/release in one, trigger on press like remotekeyboard
        if (ei_event_keyboard_get_key_is_press(event)) {
            xkb_keysym_t sym = xkb_state_key_get_one_sym(m_xkb->currentState(), xkbKey);
            Qt::KeyboardModifiers modifiers = QXkbCommon::modifiers(m_xkb->currentState(), sym);
            auto qtKey = static_cast<Qt::Key>(QXkbCommon::keysymToQtKey(sym, modifiers));
            const QString text = QXkbCommon::lookupStringNoKeysymTransformations(sym);
            Q_EMIT key(qtKey, modifiers, text);
        }
        break;
    }
    case EI_EVENT_TOUCH_DOWN:
    case EI_EVENT_TOUCH_MOTION:
    case EI_EVENT_TOUCH_UP:
    case EI_EVENT_DEVICE_PAUSED:
    case EI_EVENT_DEVICE_RESUMED:
        qCDebug(KDECONNECT_PLUGIN_SHAREINPUTDEVICES) << "Unexpected event of type" << ei_event_get_type(event);
        break;
    }
    ei_event_unref(event);
}
