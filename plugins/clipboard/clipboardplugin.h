/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef CLIPBOARDPLUGIN_H
#define CLIPBOARDPLUGIN_H

#include <QClipboard>
#include <QObject>
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

class ClipboardPlugin : public KdeConnectPlugin
{
    Q_OBJECT

public:
    explicit ClipboardPlugin(QObject *parent, const QVariantList &args);

    bool receivePacket(const NetworkPacket &np) override;
    void connected() override;
private Q_SLOTS:
    void propagateClipboard(const QString &content);
    void sendConnectPacket();
};

#endif
