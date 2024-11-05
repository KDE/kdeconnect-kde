/**
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "kdeconnectcore_export.h"
#include <QObject>

class KDECONNECTCORE_EXPORT NotificationServerInfo : public QObject
{
    Q_OBJECT

public:
    enum Hint {
        X_KDE_DISPLAY_APPNAME = 1,
        X_KDE_ORIGIN_NAME = 2,
    };

    Q_DECLARE_FLAGS(Hints, Hint)

    static NotificationServerInfo &instance();

    void init();

    Hints supportedHints();

private:
    Hints m_supportedHints;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(NotificationServerInfo::Hints)
