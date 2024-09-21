/**
 * SPDX-FileCopyrightText: 2024 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QDBusConnection>
#include <QString>

#include "kdeconnectcore_export.h"

namespace DBusHelper
{
void KDECONNECTCORE_EXPORT filterNonExportableCharacters(QString &s);
#ifdef Q_OS_MAC
int KDECONNECTCORE_EXPORT startDBusDaemon();
#endif
}
