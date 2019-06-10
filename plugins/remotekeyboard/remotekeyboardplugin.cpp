/**
 * Copyright 2017 Holger Kaelberer <holger.k@elberer.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "remotekeyboardplugin.h"
#include <KPluginFactory>
#include <QDebug>
#include <QString>
#include <QVariantMap>

K_PLUGIN_FACTORY_WITH_JSON( KdeConnectPluginFactory, "kdeconnect_remotekeyboard.json", registerPlugin< RemoteKeyboardPlugin >(); )

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_REMOTEKEYBOARD, "kdeconnect.plugin.remotekeyboard");

// Mapping of Qt::Key to internal codes, corresponds to the mapping in mousepadplugin
QMap<int, int> specialKeysMap = {
    //0,              // Invalid
    {Qt::Key_Backspace, 1},
    {Qt::Key_Tab, 2},
    //XK_Linefeed,    // 3
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
    //0,              // 17
    //0,              // 18
    //0,              // 19
    //0,              // 20
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

RemoteKeyboardPlugin::RemoteKeyboardPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
    , m_remoteState(false)
{
}

RemoteKeyboardPlugin::~RemoteKeyboardPlugin()
{
}

bool RemoteKeyboardPlugin::receivePacket(const NetworkPacket& np)
{
    if (np.type() == PACKET_TYPE_MOUSEPAD_ECHO) {
        if (!np.has(QStringLiteral("isAck")) || !np.has(QStringLiteral("key"))) {
            qCWarning(KDECONNECT_PLUGIN_REMOTEKEYBOARD) << "Invalid packet of type"
                                                        << PACKET_TYPE_MOUSEPAD_ECHO;
            return false;
        }
        //        qCWarning(KDECONNECT_PLUGIN_REMOTEKEYBOARD) << "Received keypress" << np;
        Q_EMIT keyPressReceived(np.get<QString>(QStringLiteral("key")),
                                np.get<int>(QStringLiteral("specialKey"), 0),
                                np.get<int>(QStringLiteral("shift"), false),
                                np.get<int>(QStringLiteral("ctrl"), false),
                                np.get<int>(QStringLiteral("alt"), 0));
        return true;
    } else if (np.type() == PACKET_TYPE_MOUSEPAD_KEYBOARDSTATE) {
//        qCWarning(KDECONNECT_PLUGIN_REMOTEKEYBOARD) << "Received keyboardstate" << np;
        if (m_remoteState != np.get<bool>(QStringLiteral("state"))) {
            m_remoteState = np.get<bool>(QStringLiteral("state"));
            Q_EMIT remoteStateChanged(m_remoteState);
        }
        return true;
    }
    return false;
}

void RemoteKeyboardPlugin::sendKeyPress(const QString& key, int specialKey,
                                        bool shift, bool ctrl,
                                        bool alt, bool sendAck) const
{
    NetworkPacket np(PACKET_TYPE_MOUSEPAD_REQUEST, {
                          {QStringLiteral("key"), key},
                          {QStringLiteral("specialKey"), specialKey},
                          {QStringLiteral("shift"), shift},
                          {QStringLiteral("ctrl"), ctrl},
                          {QStringLiteral("alt"), alt},
                          {QStringLiteral("sendAck"), sendAck}
                      });
    sendPacket(np);
}

void RemoteKeyboardPlugin::sendQKeyEvent(const QVariantMap& keyEvent, bool sendAck) const
{
    if (!keyEvent.contains(QStringLiteral("key")))
        return;
    int k = translateQtKey(keyEvent.value(QStringLiteral("key")).toInt());
    int modifiers = keyEvent.value(QStringLiteral("modifiers")).toInt();
    sendKeyPress(keyEvent.value(QStringLiteral("text")).toString(), k,
                 modifiers & Qt::ShiftModifier,
                 modifiers & Qt::ControlModifier,
                 modifiers & Qt::AltModifier,
                 sendAck);
}

int RemoteKeyboardPlugin::translateQtKey(int qtKey) const
{
    return specialKeysMap.value(qtKey, 0);
}

void RemoteKeyboardPlugin::connected()
{
}

QString RemoteKeyboardPlugin::dbusPath() const
{
    return QStringLiteral("/modules/kdeconnect/devices/") + device()->id() + QStringLiteral("/remotekeyboard");
}


#include "remotekeyboardplugin.moc"
