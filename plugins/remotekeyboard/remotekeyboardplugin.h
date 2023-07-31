/**
 * SPDX-FileCopyrightText: 2017 Holger Kaelberer <holger.k@elberer.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QDBusInterface>
#include <QVariantMap>
#include <core/kdeconnectplugin.h>

struct FakeKey;

#define PACKET_TYPE_MOUSEPAD_REQUEST QLatin1String("kdeconnect.mousepad.request")
#define PACKET_TYPE_MOUSEPAD_ECHO QLatin1String("kdeconnect.mousepad.echo")
#define PACKET_TYPE_MOUSEPAD_KEYBOARDSTATE QLatin1String("kdeconnect.mousepad.keyboardstate")

class RemoteKeyboardPlugin : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.remotekeyboard")
    Q_PROPERTY(bool remoteState READ remoteState NOTIFY remoteStateChanged)

private:
    bool m_remoteState;

public:
    explicit RemoteKeyboardPlugin(QObject *parent, const QVariantList &args);

    void receivePacket(const NetworkPacket &np) override;
    QString dbusPath() const override;

    bool remoteState() const
    {
        return m_remoteState;
    }

    Q_SCRIPTABLE void sendKeyPress(const QString &key, int specialKey = 0, bool shift = false, bool ctrl = false, bool alt = false, bool sendAck = true) const;
    Q_SCRIPTABLE void sendQKeyEvent(const QVariantMap &keyEvent, bool sendAck = true) const;
    Q_SCRIPTABLE int translateQtKey(int qtKey) const;

Q_SIGNALS:
    Q_SCRIPTABLE void keyPressReceived(const QString &key, int specialKey = 0, bool shift = false, bool ctrl = false, bool alt = false);
    Q_SCRIPTABLE void remoteStateChanged(bool state);
};
