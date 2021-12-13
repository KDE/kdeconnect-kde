/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "clipboardplugin.h"

#include "clipboardlistener.h"
#include "plugin_clipboard_debug.h"

#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(ClipboardPlugin, "kdeconnect_clipboard.json")

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
