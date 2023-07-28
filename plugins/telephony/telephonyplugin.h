/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QPointer>

#include <KNotification>

#include <core/kdeconnectplugin.h>

/**
 * Packet used for simple telephony events
 *
 * It contains the key "event" which maps to a string indicating the type of event:
 *  - "ringing" - A phone call is incoming
 *  - "missedCall" - An incoming call was not answered
 *
 * Historically, "sms" was a valid event, but support for that has been dropped in favour
 * of the SMS plugin's more expressive interfaces
 *
 * Depending on the event, other fields may be defined
 */
#define PACKET_TYPE_TELEPHONY QStringLiteral("kdeconnect.telephony")

#define PACKET_TYPE_TELEPHONY_REQUEST_MUTE QStringLiteral("kdeconnect.telephony.request_mute")

class TelephonyPlugin : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.telephony")

public:
    using KdeConnectPlugin::KdeConnectPlugin;

    void receivePacket(const NetworkPacket &np) override;
    QString dbusPath() const override;

public:
Q_SIGNALS:
    Q_SCRIPTABLE void callReceived(const QString &event, const QString &number, const QString contactName);

private Q_SLOTS:
    void sendMutePacket();

private:
    /**
     * Create a notification for:
     *  - Incoming call (with the option to mute the ringing)
     *  - Missed call
     */
    void createNotification(const NetworkPacket &np);

    QPointer<KNotification> m_currentCallNotification;
};
