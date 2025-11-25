/**
 * SPDX-FileCopyrightText: 2025 Martin Sh <hemisputnik@proton.me>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "abstractdigitizerimpl.h"

#include <QObject>

#include <libevdev/libevdev-uinput.h>
#include <libevdev/libevdev.h>
#include <qobject.h>

class LinuxDigitizerImpl : public AbstractDigitizerImpl
{
    Q_OBJECT

public:
    explicit LinuxDigitizerImpl(QObject *parent, const Device *device);

    void startSession(int width, int height, int resolutionX, int resolutionY) override;
    void endSession() override;

    void onToolEvent(ToolEvent event) override;

private:
    libevdev *m_dev;
    libevdev_uinput *m_uinput;
};
