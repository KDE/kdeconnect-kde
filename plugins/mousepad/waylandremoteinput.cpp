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

#include <linux/input.h>
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

RemoteDesktopSession::RemoteDesktopSession()
    : iface(new OrgFreedesktopPortalRemoteDesktopInterface(QLatin1String("org.freedesktop.portal.Desktop"),
                                                           QLatin1String("/org/freedesktop/portal/desktop"),
                                                           QDBusConnection::sessionBus(),
                                                           this))
{
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
}

void RemoteDesktopSession::handleXdpSessionFinished(uint /*code*/, const QVariantMap & /*results*/)
{
    m_xdpPath = {};
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
            s_session->iface->NotifyPointerButton(s_session->m_xdpPath, {}, BTN_LEFT, 1);
            s_session->iface->NotifyPointerButton(s_session->m_xdpPath, {}, BTN_LEFT, 0);
        } else if (isDoubleClick) {
            s_session->iface->NotifyPointerButton(s_session->m_xdpPath, {}, BTN_LEFT, 1);
            s_session->iface->NotifyPointerButton(s_session->m_xdpPath, {}, BTN_LEFT, 0);
            s_session->iface->NotifyPointerButton(s_session->m_xdpPath, {}, BTN_LEFT, 1);
            s_session->iface->NotifyPointerButton(s_session->m_xdpPath, {}, BTN_LEFT, 0);
        } else if (isMiddleClick) {
            s_session->iface->NotifyPointerButton(s_session->m_xdpPath, {}, BTN_MIDDLE, 1);
            s_session->iface->NotifyPointerButton(s_session->m_xdpPath, {}, BTN_MIDDLE, 0);
        } else if (isRightClick) {
            s_session->iface->NotifyPointerButton(s_session->m_xdpPath, {}, BTN_RIGHT, 1);
            s_session->iface->NotifyPointerButton(s_session->m_xdpPath, {}, BTN_RIGHT, 0);
        } else if (isSingleHold) {
            // For drag'n drop
            s_session->iface->NotifyPointerButton(s_session->m_xdpPath, {}, BTN_LEFT, 1);
        } else if (isSingleRelease) {
            s_session->iface->NotifyPointerButton(s_session->m_xdpPath, {}, BTN_LEFT, 0);
        } else if (isScroll) {
            s_session->iface->NotifyPointerAxis(s_session->m_xdpPath, {}, dx, dy);
        } else if (specialKey || !key.isEmpty()) {
            bool ctrl = np.get<bool>(QStringLiteral("ctrl"), false);
            bool alt = np.get<bool>(QStringLiteral("alt"), false);
            bool shift = np.get<bool>(QStringLiteral("shift"), false);
            bool super = np.get<bool>(QStringLiteral("super"), false);

            if (ctrl)
                s_session->iface->NotifyKeyboardKeycode(s_session->m_xdpPath, {}, KEY_LEFTCTRL, 1);
            if (alt)
                s_session->iface->NotifyKeyboardKeycode(s_session->m_xdpPath, {}, KEY_LEFTALT, 1);
            if (shift)
                s_session->iface->NotifyKeyboardKeycode(s_session->m_xdpPath, {}, KEY_LEFTSHIFT, 1);
            if (super)
                s_session->iface->NotifyKeyboardKeycode(s_session->m_xdpPath, {}, KEY_LEFTMETA, 1);

            if (specialKey) {
                s_session->iface->NotifyKeyboardKeycode(s_session->m_xdpPath, {}, SpecialKeysMap[specialKey], 1);
                s_session->iface->NotifyKeyboardKeycode(s_session->m_xdpPath, {}, SpecialKeysMap[specialKey], 0);
            } else if (!key.isEmpty()) {
                for (const QChar character : key) {
                    const auto keysym = xkb_utf32_to_keysym(character.unicode());
                    if (keysym != XKB_KEY_NoSymbol) {
                        s_session->iface->NotifyKeyboardKeysym(s_session->m_xdpPath, {}, keysym, 1).waitForFinished();
                        s_session->iface->NotifyKeyboardKeysym(s_session->m_xdpPath, {}, keysym, 0).waitForFinished();
                    } else {
                        qCDebug(KDECONNECT_PLUGIN_MOUSEPAD) << "Cannot send character" << character;
                    }
                }
            }

            if (ctrl)
                s_session->iface->NotifyKeyboardKeycode(s_session->m_xdpPath, {}, KEY_LEFTCTRL, 0);
            if (alt)
                s_session->iface->NotifyKeyboardKeycode(s_session->m_xdpPath, {}, KEY_LEFTALT, 0);
            if (shift)
                s_session->iface->NotifyKeyboardKeycode(s_session->m_xdpPath, {}, KEY_LEFTSHIFT, 0);
            if (super)
                s_session->iface->NotifyKeyboardKeycode(s_session->m_xdpPath, {}, KEY_LEFTMETA, 0);
        }
    } else { // Is a mouse move event
        if (dx || dy)
            s_session->iface->NotifyPointerMotion(s_session->m_xdpPath, {}, dx, dy);
        else if ((np.has(QStringLiteral("x")) || np.has(QStringLiteral("y"))))
            s_session->iface->NotifyPointerMotionAbsolute(s_session->m_xdpPath, {}, 0, x, y);
    }
    return true;
}

#include "moc_waylandremoteinput.cpp"
