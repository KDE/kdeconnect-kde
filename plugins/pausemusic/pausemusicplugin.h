/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PAUSEMUSICPLUGIN_H
#define PAUSEMUSICPLUGIN_H

#include <QObject>
#include <QSet>
#include <QString>

#include <core/kdeconnectplugin.h>

class PauseMusicPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT

public:
    explicit PauseMusicPlugin(QObject *parent, const QVariantList &args);

    bool receivePackage(const NetworkPackage& np) override;
    void connected() override { }

public Q_SLOTS:
    /**
     * @returns 0 if not muted, 1 if muted or -1 if there was an error
     */
    int isKMixMuted();
    
private:
    QSet<QString> pausedSources;
    bool muted;

};

#endif
