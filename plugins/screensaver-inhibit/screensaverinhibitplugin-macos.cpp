/**
 * SPDX-FileCopyrightText: 2019 Weixuan XIAO <veyx.shaw@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "screensaverinhibitplugin-macos.h"

#include "kdeconnect_screensaverinhibit_debug.h"
#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(ScreensaverInhibitPlugin, "kdeconnect_screensaver_inhibit.json")

ScreensaverInhibitPlugin::ScreensaverInhibitPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , m_caffeinateProcess(nullptr)
{
    if (QFile::exists(QStringLiteral("/usr/bin/caffeinate"))) {
        m_caffeinateProcess = new QProcess();
        m_caffeinateProcess->setProgram(QStringLiteral("caffeinate"));
        m_caffeinateProcess->setArguments({QStringLiteral("-d")}); // Prevent the display from sleeping
        m_caffeinateProcess->start();
    } else {
        qWarning(KDECONNECT_PLUGIN_SCREENSAVERINHIBIT) << "Cannot find caffeinate on macOS install";
    }
}

ScreensaverInhibitPlugin::~ScreensaverInhibitPlugin()
{
    if (m_caffeinateProcess != nullptr) {
        m_caffeinateProcess->terminate();
        m_caffeinateProcess = nullptr;
    }
}

bool ScreensaverInhibitPlugin::receivePacket(const NetworkPacket &np)
{
    Q_UNUSED(np);
    return false;
}

#include "moc_screensaverinhibitplugin-macos.cpp"
#include "screensaverinhibitplugin-macos.moc"
