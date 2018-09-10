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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "smsplugin.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDebug>
#include <QDBusConnection>
#include <QLoggingCategory>

#include <core/device.h>
#include <core/daemon.h>

#include "sendreplydialog.h"

K_PLUGIN_FACTORY_WITH_JSON( KdeConnectPluginFactory, "kdeconnect_sms.json", registerPlugin< SmsPlugin >(); )

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_SMS, "kdeconnect.plugin.sms")

SmsPlugin::SmsPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
    , m_telepathyInterface(QStringLiteral("org.freedesktop.Telepathy.ConnectionManager.kdeconnect"), QStringLiteral("/kdeconnect"))
    , m_conversationInterface(new ConversationsDbusInterface(this))
{
}

SmsPlugin::~SmsPlugin()
{
}

bool SmsPlugin::receivePacket(const NetworkPacket& np)
{
    if (np.get<bool>(QStringLiteral("isCancel"))) {

        //TODO: Clear the old notification
        return true;
    }

    const QString& event = np.get<QString>(QStringLiteral("event"), QStringLiteral("unknown"));

    // Handle old-style packets
    // TODO Drop support?
    if (np.type() == PACKET_TYPE_TELEPHONY)
    {
        if (event == QLatin1String("sms"))
        {
            // New-style packets should be a PACKET_TYPE_TELEPHONY_MESSAGE (15 May 2018)
            qCDebug(KDECONNECT_PLUGIN_SMS) << "Handled an old-style Telephony sms packet. You should update your Android app to get the latest features!";
            ConversationMessage message(np.body());

            // In case telepathy can handle the message, don't do anything else
            if (m_telepathyInterface.isValid()) {
                forwardToTelepathy(message);
                return true;
            }

            KNotification* n = createNotification(np);
            if (n != nullptr) n->sendEvent();
                return true;

        }
    }

    if (np.type() == PACKET_TYPE_TELEPHONY_MESSAGE) {
        return handleBatchMessages(np);
    }

    return true;
}

KNotification* SmsPlugin::createNotification(const NetworkPacket& np)
{
    const QString event = np.get<QString>(QStringLiteral("event"));
    const QString phoneNumber = np.get<QString>(QStringLiteral("phoneNumber"), i18n("unknown number"));
    const QString contactName = np.get<QString>(QStringLiteral("contactName"), phoneNumber);
    const QByteArray phoneThumbnail = QByteArray::fromBase64(np.get<QByteArray>(QStringLiteral("phoneThumbnail"), ""));
    const QString messageBody = np.get<QString>(QStringLiteral("messageBody"),{});

    const QString title = device()->name();

    QString type = QStringLiteral("smsReceived");
    QString icon = QStringLiteral("mail-receive");
    QString content = i18n("SMS from %1<br>%2", contactName, messageBody);
    KNotification::NotificationFlags flags = KNotification::Persistent; //Note that in Unity this generates a message box!

    qCDebug(KDECONNECT_PLUGIN_SMS) << "Creating notification with type:" << type;

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

    notification->setActions( QStringList(i18n("Reply")) );
    notification->setProperty("phoneNumber", phoneNumber);
    notification->setProperty("contactName", contactName);
    notification->setProperty("originalMessage", messageBody);
    connect(notification, &KNotification::action1Activated, this, &SmsPlugin::showSendSmsDialog);

    return notification;

}

void SmsPlugin::sendSms(const QString& phoneNumber, const QString& messageBody)
{
    NetworkPacket np(PACKET_TYPE_SMS_REQUEST, {
        {"sendSms", true},
        {"phoneNumber", phoneNumber},
        {"messageBody", messageBody}
    });
    qDebug() << "sending sms!";
    sendPacket(np);
}

void SmsPlugin::showSendSmsDialog()
{
    qCDebug(KDECONNECT_PLUGIN_SMS) << "Show dialog";
    QString phoneNumber = sender()->property("phoneNumber").toString();
    QString contactName = sender()->property("contactName").toString();
    QString originalMessage = sender()->property("originalMessage").toString();
    SendReplyDialog* dialog = new SendReplyDialog(originalMessage, phoneNumber, contactName);
    connect(dialog, &SendReplyDialog::sendReply, this, &SmsPlugin::sendSms);
    dialog->show();
    dialog->raise();
}

void SmsPlugin::requestAllConversations()
{
    NetworkPacket np(PACKET_TYPE_TELEPHONY_REQUEST_CONVERSATIONS);

    sendPacket(np);
}

void SmsPlugin::requestConversation (const QString& conversationID) const
{
    NetworkPacket np(PACKET_TYPE_TELEPHONY_REQUEST_CONVERSATION);
    np.set("threadID", conversationID.toInt());

    sendPacket(np);
}

void SmsPlugin::forwardToTelepathy(const ConversationMessage& message)
{
    // If we don't have a valid Telepathy interface, bail out
    if (!(m_telepathyInterface.isValid())) return;

    qCDebug(KDECONNECT_PLUGIN_SMS) << "Passing a text message to the telepathy interface";
    connect(&m_telepathyInterface, SIGNAL(messageReceived(QString,QString)), SLOT(sendSms(QString,QString)), Qt::UniqueConnection);
    const QString messageBody = message.body();
    const QString contactName; // TODO: When telepathy support is improved, look up the contact with KPeople
    const QString phoneNumber = message.address();
    m_telepathyInterface.call(QDBus::NoBlock, QStringLiteral("sendMessage"), phoneNumber, contactName, messageBody);
}

bool SmsPlugin::handleBatchMessages(const NetworkPacket& np)
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


QString SmsPlugin::dbusPath() const
{
    return "/modules/kdeconnect/devices/" + device()->id() + "/sms";
}

#include "smsplugin.moc"

