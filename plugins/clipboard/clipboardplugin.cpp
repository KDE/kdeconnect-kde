/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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

#include "clipboardplugin.h"

#include "clipboardlistener.h"

#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(ClipboardPlugin, "kdeconnect_clipboard.json")

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_CLIPBOARD, "kdeconnect.plugin.clipboard")

ClipboardPlugin::ClipboardPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
    connect(ClipboardListener::instance(), &ClipboardListener::clipboardChanged,
            this, &ClipboardPlugin::propagateClipboard);
}

void ClipboardPlugin::connected()
{
    sendConnectPacket();
}

void ClipboardPlugin::propagateClipboard(const QString& content)
{
    NetworkPacket np(PACKET_TYPE_CLIPBOARD, {{QStringLiteral("content"), content}});
    sendPacket(np);
}

void ClipboardPlugin::sendConnectPacket()
{
    NetworkPacket np(PACKET_TYPE_CLIPBOARD_CONNECT, {
        {QStringLiteral("content"), ClipboardListener::instance()->currentContent()},
        {QStringLiteral("timestamp"), ClipboardListener::instance()->updateTimestamp()}
    });
    sendPacket(np);
}


bool ClipboardPlugin::receivePacket(const NetworkPacket& np)
{
    QString content = np.get<QString>(QStringLiteral("content"));
    if (np.type() == PACKET_TYPE_CLIPBOARD) {
        ClipboardListener::instance()->setText(content);
        return true;
    } else if (np.type() == PACKET_TYPE_CLIPBOARD_CONNECT) {
        qint64 packetTime = np.get<qint64>(QStringLiteral("timestamp"));
        // If the packetTime is 0, it means the timestamp is unknown (so do nothing).
        if (packetTime == 0 || packetTime < ClipboardListener::instance()->updateTimestamp()) {
            return false;
        }

        ClipboardListener::instance()->setText(content);
        return true;
    }
    return false;
}

#include "clipboardplugin.moc"
