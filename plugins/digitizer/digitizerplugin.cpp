/**
 * SPDX-FileCopyrightText: 2025 Martin Sh <hemisputnik@proton.me>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "digitizerplugin.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDebug>
#include <QGuiApplication>

#if defined(Q_OS_LINUX)
#include "linuxdigitizerimpl.h"
#endif

#include "plugin_digitizer_debug.h"

K_PLUGIN_CLASS_WITH_JSON(DigitizerPlugin, "kdeconnect_digitizer.json")

DigitizerPlugin::DigitizerPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , m_impl(nullptr)
{
#if defined(Q_OS_LINUX)
    m_impl = new LinuxDigitizerImpl(this, device());
#endif

    if (!m_impl) {
        qWarning(KDECONNECT_PLUGIN_DIGITIZER) << "KDE Connect drawing tablet plugin was built without" << QGuiApplication::platformName() << "support.";
    }
}

DigitizerPlugin::~DigitizerPlugin()
{
    if (m_impl) {
        m_impl->endSession();
        delete m_impl;
    }
}

void DigitizerPlugin::receivePacket(const NetworkPacket &np)
{
    if (!m_impl)
        return;

    if (np.type() == PACKET_TYPE_DIGITIZER_SESSION) {
        if (!np.has(QStringLiteral("action"))) {
            qCWarning(KDECONNECT_PLUGIN_DIGITIZER) << "Received malformed session packet. Ignoring.";
            return;
        }

        if (np.get<QString>(QStringLiteral("action")) == QStringLiteral("start")) {
            if (!np.has(QStringLiteral("width")) || !np.has(QStringLiteral("height")) || !np.has(QStringLiteral("resolutionX"))
                || !np.has(QStringLiteral("resolutionY"))) {
                qCWarning(KDECONNECT_PLUGIN_DIGITIZER) << "Received malformed session start packet. Ignoring.";
                return;
            }

            m_impl->startSession(np.get<int>(QStringLiteral("width")),
                                 np.get<int>(QStringLiteral("height")),
                                 np.get<int>(QStringLiteral("resolutionX")),
                                 np.get<int>(QStringLiteral("resolutionY")));
        } else if (np.get<QString>(QStringLiteral("action")) == QStringLiteral("end")) {
            m_impl->endSession();
        }

        return;
    }

    if (np.type() == PACKET_TYPE_DIGITIZER) {
        m_impl->onToolEvent({
            .active = np.has(QStringLiteral("active")) ? std::make_optional(np.get<bool>(QStringLiteral("active"))) : std::nullopt,
            .touching = np.has(QStringLiteral("touching")) ? std::make_optional(np.get<bool>(QStringLiteral("touching"))) : std::nullopt,
            .tool = np.has(QStringLiteral("tool")) ? std::make_optional(np.get<QString>(QStringLiteral("tool"))) : std::nullopt,
            .x = np.has(QStringLiteral("x")) ? std::make_optional(np.get<int>(QStringLiteral("x"))) : std::nullopt,
            .y = np.has(QStringLiteral("y")) ? std::make_optional(np.get<int>(QStringLiteral("y"))) : std::nullopt,
            .pressure = np.has(QStringLiteral("pressure")) ? std::make_optional(np.get<double>(QStringLiteral("pressure"))) : std::nullopt,
        });
    }
}

#include "digitizerplugin.moc"
#include "moc_digitizerplugin.cpp"
