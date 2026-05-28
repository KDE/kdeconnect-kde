/**
 * SPDX-FileCopyrightText: 2017 Holger Kaelberer <holger.k@elberer.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "remotekeyboardplugin.h"
#include "plugin_remotekeyboard_debug.h"
#include "virtualkeyboardinjector.h"
#include <KPluginFactory>
#include <QDebug>
#include <QKeySequence>
#include <QString>
#include <QVariantMap>
#include <linux/input.h>

K_PLUGIN_CLASS_WITH_JSON(RemoteKeyboardPlugin, "kdeconnect_remotekeyboard.json")

// Mapping of Qt::Key to internal codes, corresponds to the mapping in mousepadplugin
QMap<int, int> specialKeysMap = {
    // 0,              // Invalid
    {Qt::Key_Backspace, 1},
    {Qt::Key_Tab, 2},
    // XK_Linefeed,    // 3
    {Qt::Key_Left, 4},
    {Qt::Key_Up, 5},
    {Qt::Key_Right, 6},
    {Qt::Key_Down, 7},
    {Qt::Key_PageUp, 8},
    {Qt::Key_PageDown, 9},
    {Qt::Key_Home, 10},
    {Qt::Key_End, 11},
    {Qt::Key_Return, 12},
    {Qt::Key_Enter, 12},
    {Qt::Key_Delete, 13},
    {Qt::Key_Escape, 14},
    {Qt::Key_SysReq, 15},
    {Qt::Key_ScrollLock, 16},
    // 0,              // 17
    // 0,              // 18
    // 0,              // 19
    // 0,              // 20
    {Qt::Key_F1, 21},
    {Qt::Key_F2, 22},
    {Qt::Key_F3, 23},
    {Qt::Key_F4, 24},
    {Qt::Key_F5, 25},
    {Qt::Key_F6, 26},
    {Qt::Key_F7, 27},
    {Qt::Key_F8, 28},
    {Qt::Key_F9, 29},
    {Qt::Key_F10, 30},
    {Qt::Key_F11, 31},
    {Qt::Key_F12, 32},
};

RemoteKeyboardPlugin::RemoteKeyboardPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , m_remoteState(false)
{
    // Initialize virtual keyboard injector for direct compositor key injection
    // This bypasses the QML/Plasma signal path that doesn't work on non-KDE compositors
    if (VirtualKeyboardInjector::isAvailable()) {
        m_keyboardInjector = new VirtualKeyboardInjector(this);
    }
}

void RemoteKeyboardPlugin::receivePacket(const NetworkPacket &np)
{
    if (np.type() == PACKET_TYPE_MOUSEPAD_ECHO) {
        if (!np.has(QStringLiteral("isAck")) || !np.has(QStringLiteral("key"))) {
            qCWarning(KDECONNECT_PLUGIN_REMOTEKEYBOARD) << "Invalid packet of type" << PACKET_TYPE_MOUSEPAD_ECHO;
            return;
        }
        QString key = np.get<QString>(QStringLiteral("key"));
        int specialKey = np.get<int>(QStringLiteral("specialKey"), 0);
        int shift = np.get<int>(QStringLiteral("shift"), false);
        int ctrl = np.get<int>(QStringLiteral("ctrl"), false);
        int alt = np.get<int>(QStringLiteral("alt"), 0);

        // Inject keyboard events
        if (m_keyboardInjector) {
            if (specialKey) {
                // Send modifiers if present
                if (ctrl)
                    m_keyboardInjector->sendKeycode(KEY_LEFTCTRL, true);
                if (alt)
                    m_keyboardInjector->sendKeycode(KEY_LEFTALT, true);
                if (shift)
                    m_keyboardInjector->sendKeycode(KEY_LEFTSHIFT, true);

                m_keyboardInjector->sendSpecialKey(specialKey);

                // Release modifiers
                if (ctrl)
                    m_keyboardInjector->sendKeycode(KEY_LEFTCTRL, false);
                if (alt)
                    m_keyboardInjector->sendKeycode(KEY_LEFTALT, false);
                if (shift)
                    m_keyboardInjector->sendKeycode(KEY_LEFTSHIFT, false);
            } else if (!key.isEmpty()) {
                // Apply modifiers from packet fields
                // Use setModifiers (explicit modifier mask) in addition to key events
                if (ctrl)
                    m_keyboardInjector->sendKeycode(KEY_LEFTCTRL, true);
                if (alt)
                    m_keyboardInjector->sendKeycode(KEY_LEFTALT, true);
                if (shift)
                    m_keyboardInjector->sendKeycode(KEY_LEFTSHIFT, true);
                m_keyboardInjector->setModifiers(shift, ctrl, alt, false);

                m_keyboardInjector->sendText(key);

                if (shift)
                    m_keyboardInjector->sendKeycode(KEY_LEFTSHIFT, false);
                if (alt)
                    m_keyboardInjector->sendKeycode(KEY_LEFTALT, false);
                if (ctrl)
                    m_keyboardInjector->sendKeycode(KEY_LEFTCTRL, false);
                m_keyboardInjector->setModifiers(false, false, false, false);
            }
        }

        // Always emit the signal for the QML UI (existing behavior)
        Q_EMIT keyPressReceived(key, specialKey, shift, ctrl, alt);
    } else if (np.type() == PACKET_TYPE_MOUSEPAD_KEYBOARDSTATE) {
        bool state = np.get<bool>(QStringLiteral("state"));
        if (m_remoteState != state) {
            m_remoteState = state;
            Q_EMIT remoteStateChanged(m_remoteState);
        }
    }
}

void RemoteKeyboardPlugin::sendKeyPress(const QString &key, int specialKey, bool shift, bool ctrl, bool alt, bool sendAck) const
{
    NetworkPacket np(PACKET_TYPE_MOUSEPAD_REQUEST,
                     {{QStringLiteral("key"), key},
                      {QStringLiteral("specialKey"), specialKey},
                      {QStringLiteral("shift"), shift},
                      {QStringLiteral("ctrl"), ctrl},
                      {QStringLiteral("alt"), alt},
                      {QStringLiteral("sendAck"), sendAck}});
    sendPacket(np);
}

void RemoteKeyboardPlugin::sendQKeyEvent(const QVariantMap &keyEvent, bool sendAck) const
{
    if (!keyEvent.contains(QStringLiteral("key"))) {
        return;
    }
    const int key = keyEvent.value(QStringLiteral("key")).toInt();
    int k = translateQtKey(key);
    int modifiers = keyEvent.value(QStringLiteral("modifiers")).toInt();

    // Qt will be calling xkb_state_key_get_utf8 to create this string.
    // As documented, it will be giving us weird strings with Ctrl combinations:
    // https://xkbcommon.org/doc/current/group__keysyms.html
    // Instead, just use QKeySequence to tell us which key that is and move on
    QString text = keyEvent.value(QStringLiteral("text")).toString();
    if (!text.isEmpty() && !text[0].isLetterOrNumber() && text != QLatin1String(" ")) {
        text = QKeySequence(key).toString().toLower();
    }

    sendKeyPress(text, k, modifiers & Qt::ShiftModifier, modifiers & Qt::ControlModifier, modifiers & Qt::AltModifier, sendAck);
}

int RemoteKeyboardPlugin::translateQtKey(int qtKey) const
{
    return specialKeysMap.value(qtKey, 0);
}

QString RemoteKeyboardPlugin::dbusPath() const
{
    return QLatin1String("/modules/kdeconnect/devices/%1/remotekeyboard").arg(device()->id());
}

#include "moc_remotekeyboardplugin.cpp"
#include "remotekeyboardplugin.moc"
