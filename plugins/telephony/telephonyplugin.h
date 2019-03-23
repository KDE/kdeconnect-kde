/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
 * Copyright 2018 Simon Redman <simon@ergotech.com>
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

#ifndef TELEPHONYPLUGIN_H
#define TELEPHONYPLUGIN_H

#include <QLoggingCategory>
#include <QDBusInterface>

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

Q_DECLARE_LOGGING_CATEGORY(KDECONNECT_PLUGIN_TELEPHONY)

class TelephonyPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.telephony")

public:
    explicit TelephonyPlugin(QObject* parent, const QVariantList& args);
    ~TelephonyPlugin() override;

    bool receivePacket(const NetworkPacket& np) override;
    void connected() override {}
    QString dbusPath() const override;

public:
Q_SIGNALS:
    Q_SCRIPTABLE void callReceived(const QString& event, const QString& number, const QString contactName);

private Q_SLOTS:
    void sendMutePacket();

private:
    /**
     * Create a notification for:
     *  - Incoming call (with the option to mute the ringing)
     *  - Missed call
     */
    KNotification* createNotification(const NetworkPacket& np);
};

#endif
