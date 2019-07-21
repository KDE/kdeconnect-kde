/**
 * Copyright 2019 Aleix Pol Gonzalez <aleixpol@kde.org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PRESENTERPLUGIN_H
#define PRESENTERPLUGIN_H

#include <QObject>
#include <QPointer>
#include <QTimer>

#include <core/kdeconnectplugin.h>

#define PACKET_TYPE_PRESENTER QStringLiteral("kdeconnect.presenter")

class PresenterView;

class Q_DECL_EXPORT PresenterPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.presenter")

public:
    explicit PresenterPlugin(QObject* parent, const QVariantList& args);
    ~PresenterPlugin() override;

    bool receivePacket(const NetworkPacket& np) override;
    void connected() override {}

private:
    QPointer<PresenterView> m_view;
    QTimer* m_timer;
    float m_xPos, m_yPos;
};

#endif
