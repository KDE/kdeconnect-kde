/**
 * SPDX-FileCopyrightText: 2022 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>

#include "kdeconnectcore_export.h"

class KDECONNECTCORE_EXPORT OpenConfig : public QObject
{
    Q_OBJECT
public:
    void setXdgActivationToken(const QString &token);
    Q_INVOKABLE void openConfiguration(const QString &deviceId = QString(), const QString &pluginName = QString());

private:
    QString m_currentToken;
};
