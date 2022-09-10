/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef PAUSEMUSICPLUGIN_H
#define PAUSEMUSICPLUGIN_H

#include <QObject>
#include <QSet>
#include <QString>

#include <core/kdeconnectplugin.h>

class PauseMusicPlugin : public KdeConnectPlugin
{
    Q_OBJECT

public:
    explicit PauseMusicPlugin(QObject *parent, const QVariantList &args);

    bool receivePacket(const NetworkPacket &np) override;
    void connected() override
    {
    }

private:
    QSet<QString> pausedSources;
    QSet<QString> mutedSinks;
};

#endif
