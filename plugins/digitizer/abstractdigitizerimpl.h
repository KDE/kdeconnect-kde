/**
 * SPDX-FileCopyrightText: 2025 Martin Sh <hemisputnik@proton.me>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>

#include "device.h"

#include "toolevent.h"

class AbstractDigitizerImpl : public QObject
{
    Q_OBJECT

public:
    explicit AbstractDigitizerImpl(QObject *parent, const Device *device);

    virtual void startSession(int width, int height, int resolutionX, int resolutionY) = 0;
    virtual void endSession() = 0;

    virtual void onToolEvent(ToolEvent event) = 0;

protected:
    const Device *device() const;

private:
    const Device *m_device;
};
