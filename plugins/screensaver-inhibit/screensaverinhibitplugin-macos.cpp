/**
 * SPDX-FileCopyrightText: 2019 Weixuan XIAO <veyx.shaw@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "screensaverinhibitplugin-macos.h"

#include "plugin_screensaver_inhibit_debug.h"
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
        qWarning(KDECONNECT_PLUGIN_SCREENSAVER_INHIBIT) << "Cannot find caffeinate on macOS install";
    }
}

ScreensaverInhibitPlugin::~ScreensaverInhibitPlugin()
{
    if (m_caffeinateProcess != nullptr) {
        m_caffeinateProcess->terminate();
        m_caffeinateProcess = nullptr;
    }
}

#include "moc_screensaverinhibitplugin-macos.cpp"
#include "screensaverinhibitplugin-macos.moc"
