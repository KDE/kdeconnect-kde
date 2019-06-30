/**
 * Copyright 2019 Weixuan XIAO <veyx.shaw@gmail.com>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "screensaverinhibitplugin-macos.h"

#include <KPluginFactory>
#include <QLoggingCategory>

K_PLUGIN_CLASS_WITH_JSON(ScreensaverInhibitPlugin, "kdeconnect_screensaver_inhibit.json")

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_SCREENSAVERINHIBIT, "kdeconnect.plugin.screensaverinhibit")

ScreensaverInhibitPlugin::ScreensaverInhibitPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args), m_caffeinateProcess(nullptr)
{
    if (QFile::exists(QStringLiteral("/usr/bin/caffeinate"))) {
        m_caffeinateProcess = new QProcess();
        m_caffeinateProcess->setProgram(QStringLiteral("caffeinate"));
        m_caffeinateProcess->setArguments({QStringLiteral("-d")});      // Prevent the display from sleeping
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

void ScreensaverInhibitPlugin::connected()
{
}

bool ScreensaverInhibitPlugin::receivePacket(const NetworkPacket& np)
{
    Q_UNUSED(np);
    return false;
}

#include "screensaverinhibitplugin-macos.moc"
