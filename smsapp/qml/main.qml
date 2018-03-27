/*
 * This file is part of KDE Telepathy Chat
 *
 * Copyright (C) 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

import QtQuick 2.1
import org.kde.kirigami 2.2 as Kirigami
 
Kirigami.ApplicationWindow
{
    id: root
    visible: true
    width: 800
    height: 600

    header: Kirigami.ToolBarApplicationHeader {}

    pageStack.initialPage: ContactList {
        title: i18n("KDE Connect SMS")
    }
}
