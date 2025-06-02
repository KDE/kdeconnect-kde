/*
 * SPDX-FileCopyrightText: 2024 Carl Schwan <carl@carlschwan.eu>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>
#include <qqmlregistration.h>

class FindThisDeviceHelper : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    static Q_INVOKABLE QString defaultSound();
    Q_INVOKABLE bool pathExists(const QString &path);
};
