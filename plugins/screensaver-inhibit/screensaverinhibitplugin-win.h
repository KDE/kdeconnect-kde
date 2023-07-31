/**
 * SPDX-FileCopyrightText: 2019 Piyush Aggarwal <piyushaggarwal002@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>

#include <core/kdeconnectplugin.h>

class ScreensaverInhibitPlugin : public KdeConnectPlugin
{
    Q_OBJECT

public:
    explicit ScreensaverInhibitPlugin(QObject *parent, const QVariantList &args);
    ~ScreensaverInhibitPlugin() override;
};
