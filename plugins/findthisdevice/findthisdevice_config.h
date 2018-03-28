/**
 * Copyright 2018 Friedrich W. H. Kossebau <kossebau@kde.org>
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

#ifndef FINDTHISDEVICE_CONFIG_H
#define FINDTHISDEVICE_CONFIG_H

#include <kcmplugin/kdeconnectpluginkcm.h>

namespace Ui {
class FindThisDeviceConfigUi;
}

class FindThisDeviceConfig
    : public KdeConnectPluginKcm
{
    Q_OBJECT
public:
    FindThisDeviceConfig(QWidget* parent, const QVariantList&);
    ~FindThisDeviceConfig() override;

public Q_SLOTS:
    void save() override;
    void load() override;
    void defaults() override;

private Q_SLOTS:
    void playSound();

private:
    Ui::FindThisDeviceConfigUi* m_ui;
};

#endif
