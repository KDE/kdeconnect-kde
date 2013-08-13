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

#include <QClipboard>
#include <KDebug>
#include <QApplication>

K_PLUGIN_FACTORY( KdeConnectPluginFactory, registerPlugin< ClipboardPlugin >(); )
K_EXPORT_PLUGIN( KdeConnectPluginFactory("kdeconnect_clipboard", "kdeconnect_clipboard") )

ClipboardPlugin::ClipboardPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
{
    clipboard = QApplication::clipboard();
    ignore_next_clipboard_change = false;
    connect(clipboard,SIGNAL(changed(QClipboard::Mode)),this,SLOT(clipboardChanged(QClipboard::Mode)));
}

void ClipboardPlugin::clipboardChanged(QClipboard::Mode mode)
{
    if (mode != QClipboard::QClipboard::Clipboard) return;

    if (ignore_next_clipboard_change) {
        ignore_next_clipboard_change = false;
        return;
    }
    //qDebug() << "ClipboardChanged";
    NetworkPackage np(PACKAGE_TYPE_CLIPBOARD);
    np.set("content",clipboard->text());
    device()->sendPackage(np);
}

bool ClipboardPlugin::receivePackage(const NetworkPackage& np)
{
    if (np.type() == PACKAGE_TYPE_CLIPBOARD) {
        ignore_next_clipboard_change = true;
        clipboard->setText(np.get<QString>("content"));
        return true;
    }
    return false;
}
