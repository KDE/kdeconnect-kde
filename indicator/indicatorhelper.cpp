/*
 * SPDX-FileCopyrightText: 2019 Weixuan XIAO <veyx.shaw@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "indicatorhelper.h"

#include <QApplication>
#include <QIcon>

IndicatorHelper::IndicatorHelper()
{
}

IndicatorHelper::~IndicatorHelper()
{
}

void IndicatorHelper::iconPathHook()
{
}

int IndicatorHelper::startDaemon()
{
    return 0;
}

void IndicatorHelper::systrayIconHook(KStatusNotifierItem &systray)
{
    systray.setIconByName(QStringLiteral("kdeconnectindicatordark"));
}
