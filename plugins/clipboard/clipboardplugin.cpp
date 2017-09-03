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

#include "clipboardplugin.h"

#include "clipboardlistener.h"

#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON( KdeConnectPluginFactory, "kdeconnect_clipboard.json", registerPlugin< ClipboardPlugin >(); )

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_CLIPBOARD, "kdeconnect.plugin.clipboard")

ClipboardPlugin::ClipboardPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
    connect(ClipboardListener::instance(), &ClipboardListener::clipboardChanged,
            this, &ClipboardPlugin::propagateClipboard);
}

void ClipboardPlugin::propagateClipboard(const QString& content)
{
    NetworkPackage np(PACKAGE_TYPE_CLIPBOARD, {{"content", content}});
    sendPackage(np);
}

bool ClipboardPlugin::receivePackage(const NetworkPackage& np)
{
    QString content = np.get<QString>(QStringLiteral("content"));
    ClipboardListener::instance()->setText(content);
    return true;
}

#include "clipboardplugin.moc"
