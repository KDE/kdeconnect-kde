/**
 * SPDX-FileCopyrightText: 2019 Piyush Aggarwal <piyushaggarwal002@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "screensaverinhibitplugin-win.h"

#include <KPluginFactory>
#include <Windows.h>

K_PLUGIN_CLASS_WITH_JSON(ScreensaverInhibitPlugin, "kdeconnect_screensaver_inhibit.json")

ScreensaverInhibitPlugin::ScreensaverInhibitPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
{
    SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED);
}

ScreensaverInhibitPlugin::~ScreensaverInhibitPlugin()
{
    SetThreadExecutionState(ES_CONTINUOUS);
}

#include "moc_screensaverinhibitplugin-win.cpp"
#include "screensaverinhibitplugin-win.moc"
