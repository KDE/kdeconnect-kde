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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "telephonyplugin.h"

#include "sendreplydialog.h"
#include "conversationsdbusinterface.h"
#include "interfaces/conversationmessage.h"

#include <KLocalizedString>
#include <QDebug>
#include <QDBusReply>

#include <KPluginFactory>
#include <KNotification>

K_PLUGIN_FACTORY_WITH_JSON( KdeConnectPluginFactory, "kdeconnect_telephony.json", registerPlugin< TelephonyPlugin >(); )

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_TELEPHONY, "kdeconnect.plugin.telephony")

TelephonyPlugin::TelephonyPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
    , m_telepathyInterface(QStringLiteral("org.freedesktop.Telepathy.ConnectionManager.kdeconnect"), QStringLiteral("/kdeconnect"))
    , m_conversationInterface(new ConversationsDbusInterface(this))
{
}

TelephonyPlugin::~TelephonyPlugin()
{
    // FIXME: Same problem as discussed in the BatteryPlugin destructor and for the same reason:
    // QtDbus does not allow us to delete m_conversationInterface. If we do so, we get a crash in the
    // next DBus access to the parent

    //m_conversationInterface->deleteLater();
}

KNotification* TelephonyPlugin::createNotification(const NetworkPacket& np)
{
    const QString event = np.get<QString>(QStringLiteral("event"));
    const QString phoneNumber = np.get<QString>(QStringLiteral("phoneNumber"), i18n("unknown number"));
    const QString contactName = np.get<QString>(QStringLiteral("contactName"), phoneNumber);
    const QByteArray phoneThumbnail = QByteArray::fromBase64(np.get<QByteArray>(QStringLiteral("phoneThumbnail"), ""));
    const QString messageBody = np.get<QString>(QStringLiteral("messageBody"),{});

    // In case telepathy can handle the message, don't do anything else
    if (event == QLatin1String("sms") && m_telepathyInterface.isValid()) {
        // Telepathy has already been tried (in receivePacket)
        // There is a chance that it somehow failed, but since nobody uses Telepathy anyway...
        // TODO: When upgrading telepathy, handle failure case (in case m_telepathyInterface.call returns false)
        return nullptr;
    }

    QString content, type, icon;
    KNotification::NotificationFlags flags = KNotification::CloseOnTimeout;

    const QString title = device()->name();

    if (event == QLatin1String("ringing")) {
        type = QStringLiteral("callReceived");
        icon = QStringLiteral("call-start");
        content = i18n("Incoming call from %1", contactName);
    } else if (event == QLatin1String("missedCall")) {
        type = QStringLiteral("missedCall");
        icon = QStringLiteral("call-start");
        content = i18n("Missed call from %1", contactName);
        flags |= KNotification::Persistent; //Note that in Unity this generates a message box!
    } else if (event == QLatin1String("sms")) {
        type = QStringLiteral("smsReceived");
        icon = QStringLiteral("mail-receive");
        QString messageBody = np.get<QString>(QStringLiteral("messageBody"), QLatin1String(""));
        content = i18n("SMS from %1<br>%2", contactName, messageBody);
        flags |= KNotification::Persistent; //Note that in Unity this generates a message box!
    } else if (event == QLatin1String("talking")) {
        return nullptr;
    } else {
#ifndef NDEBUG
        return nullptr;
#else
        type = QStringLiteral("callReceived");
        icon = QStringLiteral("phone");
        content = i18n("Unknown telephony event: %1", event);
#endif
    }

    qCDebug(KDECONNECT_PLUGIN_TELEPHONY) << "Creating notification with type:" << type;

    KNotification* notification = new KNotification(type, flags, this);
    if (!phoneThumbnail.isEmpty()) {
        QPixmap photo;
        photo.loadFromData(phoneThumbnail, "JPEG");
        notification->setPixmap(photo);
    } else {
        notification->setIconName(icon);
    }
    notification->setComponentName(QStringLiteral("kdeconnect"));
    notification->setTitle(title);
    notification->setText(content);

    if (event == QLatin1String("ringing")) {
        notification->setActions( QStringList(i18n("Mute Call")) );
        connect(notification, &KNotification::action1Activated, this, &TelephonyPlugin::sendMutePacket);
    } else if (event == QLatin1String("sms")) {
        notification->setActions( QStringList(i18n("Reply")) );
        notification->setProperty("phoneNumber", phoneNumber);
        notification->setProperty("contactName", contactName);
        notification->setProperty("originalMessage", messageBody);
        connect(notification, &KNotification::action1Activated, this, &TelephonyPlugin::showSendSmsDialog);
    }

    return notification;

}

bool TelephonyPlugin::receivePacket(const NetworkPacket& np)
{
    if (np.get<bool>(QStringLiteral("isCancel"))) {

        //TODO: Clear the old notification
        return true;
    }

    const QString& event = np.get<QString>(QStringLiteral("event"), QStringLiteral("unknown"));

    // Handle old-style packets
    if (np.type() == PACKET_TYPE_TELEPHONY)
    {
        if (event == QLatin1String("sms"))
        {
            // New-style packets should be a PACKET_TYPE_TELEPHONY_MESSAGE (15 May 2018)
            qCDebug(KDECONNECT_PLUGIN_TELEPHONY) << "Handled an old-style Telephony sms packet. You should update your Android app to get the latest features!";
            ConversationMessage message(np.body());
            forwardToTelepathy(message);
        }
        KNotification* n = createNotification(np);
        if (n != nullptr) n->sendEvent();
        return true;
    }

    if (np.type() == PACKET_TYPE_TELEPHONY_MESSAGE)
    {
        return handleBatchMessages(np);
    }

    return true;
}

void TelephonyPlugin::sendMutePacket()
{
    NetworkPacket packet(PACKET_TYPE_TELEPHONY_REQUEST, {{"action", "mute"}});
    sendPacket(packet);
}

void TelephonyPlugin::sendSms(const QString& phoneNumber, const QString& messageBody)
{
    NetworkPacket np(PACKET_TYPE_SMS_REQUEST, {
        {"sendSms", true},
        {"phoneNumber", phoneNumber},
        {"messageBody", messageBody}
    });
    qDebug() << "sending sms!";
    sendPacket(np);
}

void TelephonyPlugin::showSendSmsDialog()
{
    QString phoneNumber = sender()->property("phoneNumber").toString();
    QString contactName = sender()->property("contactName").toString();
    QString originalMessage = sender()->property("originalMessage").toString();
    SendReplyDialog* dialog = new SendReplyDialog(originalMessage, phoneNumber, contactName);
    connect(dialog, &SendReplyDialog::sendReply, this, &TelephonyPlugin::sendSms);
    dialog->show();
    dialog->raise();
}

void TelephonyPlugin::requestAllConversations()
{
    NetworkPacket np(PACKET_TYPE_TELEPHONY_REQUEST_CONVERSATIONS);

    sendPacket(np);
}

void TelephonyPlugin::requestConversation (const QString& conversationID) const
{
    NetworkPacket np(PACKET_TYPE_TELEPHONY_REQUEST_CONVERSATION);
    np.set("threadID", conversationID.toInt());

    sendPacket(np);
}

void TelephonyPlugin::forwardToTelepathy(const ConversationMessage& message)
{
    // In case telepathy can handle the message, don't do anything else
    if (m_telepathyInterface.isValid()) {
        qCDebug(KDECONNECT_PLUGIN_TELEPHONY) << "Passing a text message to the telepathy interface";
        connect(&m_telepathyInterface, SIGNAL(messageReceived(QString,QString)), SLOT(sendSms(QString,QString)), Qt::UniqueConnection);
        const QString messageBody = message.body();
        const QString contactName; // TODO: When telepathy support is improved, look up the contact with KPeople
        const QString phoneNumber = message.address();
        m_telepathyInterface.call(QDBus::NoBlock, QStringLiteral("sendMessage"), phoneNumber, contactName, messageBody);
    }
}

bool TelephonyPlugin::handleBatchMessages(const NetworkPacket& np)
{
    const auto messages = np.get<QVariantList>("messages");

    for (const QVariant& body : messages)
    {
        ConversationMessage message(body.toMap());
        forwardToTelepathy(message);
        m_conversationInterface->addMessage(message);
    }

    return true;
}

QString TelephonyPlugin::dbusPath() const
{
    return "/modules/kdeconnect/devices/" + device()->id() + "/telephony";
}

#include "telephonyplugin.moc"
