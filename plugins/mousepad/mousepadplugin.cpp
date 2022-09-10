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

#if HAVE_WINDOWS
#include "windowsremoteinput.h"
#elif HAVE_MACOS
#include "macosremoteinput.h"
#else
#if HAVE_X11
#include "x11remoteinput.h"
#endif
#if HAVE_WAYLAND
#include "waylandremoteinput.h"
#endif
#endif

K_PLUGIN_CLASS_WITH_JSON(MousepadPlugin, "kdeconnect_mousepad.json")

MousepadPlugin::MousepadPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , m_impl(nullptr)
{
#if HAVE_WINDOWS
    m_impl = new WindowsRemoteInput(this);
#elif HAVE_MACOS
    m_impl = new MacOSRemoteInput(this);
#else
#if HAVE_X11
    if (QGuiApplication::platformName() == QLatin1String("xcb")) {
        m_impl = new X11RemoteInput(this);
    }
#endif

#if HAVE_WAYLAND
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive)) {
        m_impl = new WaylandRemoteInput(this);
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

bool MousepadPlugin::receivePacket(const NetworkPacket &np)
{
    if (m_impl) {
        return m_impl->handlePacket(np);
    } else {
        return false;
    }
}

void MousepadPlugin::connected()
{
    NetworkPacket np(PACKET_TYPE_MOUSEPAD_KEYBOARDSTATE);
    if (m_impl) {
        np.set<bool>(QStringLiteral("state"), m_impl->hasKeyboardSupport());
    }
    sendPacket(np);
}

#include "mousepadplugin.moc"
