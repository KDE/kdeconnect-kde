/**
 * SPDX-FileCopyrightText: 2018 Albert Vaca Cintora <albertvaka@gmail.com>
 * SPDX-FileCopyrightText: 2015 Martin Gräßlin <mgraesslin@kde.org>
 * SPDX-FileCopyrightText: 2014 Ahmed I. Khalil <ahmedibrahimkhali@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "mousepadplugin.h"
#include <KLocalizedString>
#include <KPluginFactory>
#include <QGuiApplication>

#if defined(Q_OS_WIN)
#include "windowsremoteinput.h"
#elif defined(Q_OS_MACOS)
#include "macosremoteinput.h"
#else
#include "waylandremoteinput.h"
#if WITH_X11
#include "x11remoteinput.h"
#endif
#endif

K_PLUGIN_CLASS_WITH_JSON(MousepadPlugin, "kdeconnect_mousepad.json")

MousepadPlugin::MousepadPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , m_impl(nullptr)
{
#if defined(Q_OS_WIN)
    m_impl = new WindowsRemoteInput(this);
#elif defined(Q_OS_MACOS)
    m_impl = new MacOSRemoteInput(this);
#else
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive)) {
        m_impl = new WaylandRemoteInput(this);
    }
#if WITH_X11
    if (QGuiApplication::platformName() == QLatin1String("xcb")) {
        m_impl = new X11RemoteInput(this);
    }
#endif
#endif

    if (!m_impl) {
        qDebug() << "KDE Connect was built without" << QGuiApplication::platformName() << "support";
    }
}

MousepadPlugin::~MousepadPlugin()
{
    delete m_impl;
}

void MousepadPlugin::receivePacket(const NetworkPacket &np)
{
    if (m_impl) {
        m_impl->handlePacket(np);
    }
}

void MousepadPlugin::connected()
{
    NetworkPacket np(PACKET_TYPE_MOUSEPAD_KEYBOARDSTATE);
    if (m_impl) {
        np.set<bool>(QStringLiteral("state"), m_impl->hasKeyboardSupport());
    }
    np.set<bool>(QStringLiteral("singlerelease_support"), true);
    sendPacket(np);
}

#include "moc_mousepadplugin.cpp"
#include "mousepadplugin.moc"
