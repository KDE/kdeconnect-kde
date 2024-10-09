/**
 * SPDX-FileCopyrightText: 2019 Richard Liebscher <richard.liebscher@gmail.com>
 * SPDX-FileCopyrightText: 2022 Yoram Bar Haim <bhyoram@protonmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-KDE-Accepted-GPL
 */

#include "mmtelephonyplugin.h"

#include <KLocalizedString>
#include <QDebug>
#include <QLoggingCategory>

#include "plugin_mmtelephony_debug.h"
#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(MMTelephonyPlugin, "kdeconnect_mmtelephony.json")

static const QString PACKET_TYPE_TELEPHONY = QStringLiteral("kdeconnect.telephony");
static const QString PACKET_TYPE_TELEPHONY_REQUEST_MUTE = QStringLiteral("kdeconnect.telephony.request_mute");

QSharedPointer<ModemManager::ModemVoice> _voiceInterface(const QSharedPointer<ModemManager::ModemDevice> modemDevice)
{
    return modemDevice->interface(ModemManager::ModemDevice::VoiceInterface).objectCast<ModemManager::ModemVoice>();
}

MMTelephonyPlugin::MMTelephonyPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
{
    connect(ModemManager::notifier(), &ModemManager::Notifier::modemAdded, this, &MMTelephonyPlugin::onModemAdded);
}

void MMTelephonyPlugin::receivePacket(const NetworkPacket &np)
{
    if (np.get<QString>(QStringLiteral("event")) == QLatin1String("mute")) {
        // TODO: mute code
    }
}

void MMTelephonyPlugin::onModemAdded(const QString &path)
{
    auto modemDevice = ModemManager::findModemDevice(path);
    QSharedPointer<ModemManager::ModemVoice> vcm = _voiceInterface(modemDevice);
    auto voice = vcm.get();
    connect(voice, &ModemManager::ModemVoice::callAdded, this, [this, voice](const QString &uni) {
        auto call = voice->findCall(uni);
        onCallAdded(call);
    });
    connect(voice, &ModemManager::ModemVoice::callDeleted, this, [this, voice](const QString &uni) {
        auto call = voice->findCall(uni);
        onCallRemoved(call);
    });
}

void MMTelephonyPlugin::onCallAdded(ModemManager::Call::Ptr call)
{
    qCDebug(KDECONNECT_PLUGIN_MMTELEPHONY) << "Call added" << call->number();

    connect(call.get(), &ModemManager::Call::stateChanged, this, [=, this](MMCallState newState, MMCallState oldState) {
        onCallStateChanged(call.get(), newState, oldState);
    });
}

void MMTelephonyPlugin::onCallRemoved(ModemManager::Call::Ptr call)
{
    qCDebug(KDECONNECT_PLUGIN_MMTELEPHONY) << "Call removed" << call.get()->number();
}

QString MMTelephonyPlugin::stateName(MMCallState state)
{
    QString event;
    switch (state) {
    case MMCallState::MM_CALL_STATE_RINGING_IN:
        event = QStringLiteral("ringing");
        break;
    case MMCallState::MM_CALL_STATE_ACTIVE:
        event = QStringLiteral("talking");
        break;
    case MMCallState::MM_CALL_STATE_TERMINATED:
        event = QStringLiteral("disconnected");
        break;
    case MMCallState::MM_CALL_STATE_UNKNOWN:
    default:
        event = QStringLiteral("Unknown");
    }
    return event;
}

void MMTelephonyPlugin::onCallStateChanged(ModemManager::Call *call, MMCallState newState, MMCallState oldState)
{
    auto event = stateName(newState);

    qCDebug(KDECONNECT_PLUGIN_MMTELEPHONY) << "Call state changed" << call->uni() << event;
    if (newState != MMCallState::MM_CALL_STATE_TERMINATED)
        sendMMTelephonyPacket(call, event);
    else
        sendCancelMMTelephonyPacket(call, stateName(oldState));
}

void MMTelephonyPlugin::sendMMTelephonyPacket(ModemManager::Call *call, const QString &state)
{
    QString phoneNumber = call->number();

    qCDebug(KDECONNECT_PLUGIN_MMTELEPHONY) << "Phone number is" << phoneNumber;
    NetworkPacket np{PACKET_TYPE_TELEPHONY,
                     {
                         {QStringLiteral("event"), state},
                         {QStringLiteral("phoneNumber"), phoneNumber},
                         {QStringLiteral("contactName"), phoneNumber},
                     }};
    sendPacket(np);
}

void MMTelephonyPlugin::sendCancelMMTelephonyPacket(ModemManager::Call *call, const QString &lastState)
{
    QString phoneNumber = call->number();

    NetworkPacket np{PACKET_TYPE_TELEPHONY,
                     {{QStringLiteral("event"), lastState},
                      {QStringLiteral("phoneNumber"), phoneNumber},
                      {QStringLiteral("contactName"), phoneNumber},
                      {QStringLiteral("isCancel"), true}}};
    sendPacket(np);
}

#include "mmtelephonyplugin.moc"
#include "moc_mmtelephonyplugin.cpp"
