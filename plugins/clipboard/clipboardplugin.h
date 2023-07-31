/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QClipboard>
#include <QObject>
#include <core/kdeconnectplugin.h>

#include "clipboardlistener.h"

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
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.clipboard")
    Q_PROPERTY(bool isAutoShareDisabled READ isAutoShareDisabled NOTIFY autoShareDisabledChanged)
public:
    explicit ClipboardPlugin(QObject *parent, const QVariantList &args);

    Q_SCRIPTABLE void sendClipboard();
    Q_SCRIPTABLE void sendClipboard(const QString &content);
    QString dbusPath() const override;

    void receivePacket(const NetworkPacket &np) override;
    void connected() override;
    bool isAutoShareDisabled();

Q_SIGNALS:
    Q_SCRIPTABLE void autoShareDisabledChanged(bool b);

private Q_SLOTS:
    void clipboardChanged(const QString &content, ClipboardListener::ClipboardContentType contentType);
    void sendConnectPacket();
    void configChanged();

private:
    bool autoShare;
    bool sharePasswords;
};
