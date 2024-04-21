/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 * SPDX-FileCopyrightText: 2018 Simon Redman <simon@ergotech.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "telephonyplugin.h"

#include <KLocalizedString>
#include <QDBusReply>
#include <QDebug>

#include "plugin_telephony_debug.h"
#include <KNotification>
#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(TelephonyPlugin, "kdeconnect_telephony.json")

void TelephonyPlugin::createNotification(const NetworkPacket &np)
{
    const QString event = np.get<QString>(QStringLiteral("event"));
    const QString phoneNumber = np.get<QString>(QStringLiteral("phoneNumber"), i18n("unknown number"));
    const QString contactName = np.get<QString>(QStringLiteral("contactName"), phoneNumber);
    const QByteArray phoneThumbnail = QByteArray::fromBase64(np.get<QByteArray>(QStringLiteral("phoneThumbnail"), ""));

    QString content, type, icon;

    if (event == QLatin1String("ringing")) {
        type = QStringLiteral("callReceived");
        icon = QStringLiteral("call-start");
        content = i18n("Incoming call from %1", contactName);
    } else if (event == QLatin1String("missedCall")) {
        type = QStringLiteral("missedCall");
        icon = QStringLiteral("call-start");
        content = i18n("Missed call from %1", contactName);
    } else if (event == QLatin1String("talking")) {
        if (m_currentCallNotification) {
            m_currentCallNotification->close();
        }
        return;
    }

    Q_EMIT callReceived(type, phoneNumber, contactName);

    qCDebug(KDECONNECT_PLUGIN_TELEPHONY) << "Creating notification with type:" << type;

    if (!m_currentCallNotification) {
        m_currentCallNotification = new KNotification(type, KNotification::Persistent);
    }

    if (!phoneThumbnail.isEmpty()) {
        QPixmap photo;
        photo.loadFromData(phoneThumbnail, "JPEG");
        m_currentCallNotification->setPixmap(photo);
    } else {
        m_currentCallNotification->setIconName(icon);
    }
    m_currentCallNotification->setComponentName(QStringLiteral("kdeconnect"));
    m_currentCallNotification->setTitle(device()->name());
    m_currentCallNotification->setText(content);

    if (event == QLatin1String("ringing")) {
        m_currentCallNotification->clearActions();
        KNotificationAction *muteAction = m_currentCallNotification->addAction(i18n("Mute Call"));
        connect(muteAction, &KNotificationAction::activated, this, &TelephonyPlugin::sendMutePacket);
    }

    m_currentCallNotification->sendEvent();
}

void TelephonyPlugin::receivePacket(const NetworkPacket &np)
{
    if (np.get<bool>(QStringLiteral("isCancel"))) {
        if (m_currentCallNotification) {
            m_currentCallNotification->close();
        }
        return;
    }

    // ignore old style sms packet
    if (np.get<QString>(QStringLiteral("event")) != QLatin1String("sms")) {
        createNotification(np);
    }
}

void TelephonyPlugin::sendMutePacket()
{
    NetworkPacket packet(PACKET_TYPE_TELEPHONY_REQUEST_MUTE, {{QStringLiteral("action"), QStringLiteral("mute")}});
    sendPacket(packet);
}

QString TelephonyPlugin::dbusPath() const
{
    return QLatin1String("/modules/kdeconnect/devices/%1/telepony").arg(device()->id());
}

#include "moc_telephonyplugin.cpp"
#include "telephonyplugin.moc"
