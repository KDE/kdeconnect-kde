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

#ifndef CLIPBOARDPLUGIN_H
#define CLIPBOARDPLUGIN_H

#include <QObject>
#include <QClipboard>
#include <core/kdeconnectplugin.h>

/**
 * Packet containing just clipboard contents, sent when a device updates its clipboard.
 * <p>
 * The body should look like so:
 * {
 * "content": "password"
 * }
 */
#define PACKET_TYPE_CLIPBOARD QStringLiteral("kdeconnect.clipboard")

/**
 * Packet containing clipboard contents and a timestamp that the contents were last updated, sent
 * on first connection
 * <p>
 * The timestamp is milliseconds since epoch. It can be 0, which indicates that the clipboard
 * update time is currently unknown.
 * <p>
 * The body should look like so:
 * {
 * "timestamp": 542904563213,
 * "content": "password"
 * }
 */
#define PACKET_TYPE_CLIPBOARD_CONNECT QStringLiteral("kdeconnect.clipboard.connect")

class ClipboardPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT

public:
    explicit ClipboardPlugin(QObject* parent, const QVariantList& args);

    bool receivePacket(const NetworkPacket& np) override;
    void connected() override;
private Q_SLOTS:
    void propagateClipboard(const QString& content);
    void sendConnectPacket();

};

#endif
