/**
 * SPDX-FileCopyrightText: 2024 David Redondo <kde@david-redondo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QLine>
#include <QObject>
#include <QPointF>

#include <core/kdeconnectplugin.h>

class InputCaptureSession;
class QScreen;

class ShareInputDevicesPlugin : public KdeConnectPlugin
{
    Q_OBJECT
public:
    ShareInputDevicesPlugin(QObject *parent, const QVariantList &args);

    void receivePacket(const NetworkPacket &np) override;
    QString dbusPath() const override;

private:
    Qt::Edge configuredEdge() const;
    InputCaptureSession *m_inputCaptureSession;
    QLine m_activatedBarrier;
};
