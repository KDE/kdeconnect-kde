/*
 * Copyright (C) 2010 Jacopo De Simoi <wilderkde@gmail.com>
 *
 * This program is free software you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/


#ifndef DEVICENOTIFICATIONSENGINE_H
#define DEVICENOTIFICATIONSENGINE_H

#include <Plasma/DataEngine>

/**
 *  Engine which provides data sources for device notifications.
 *  Each notification is represented by one source.
 */
class KdeConnectEngine : public Plasma::DataEngine
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.plasmoid")

public:
    KdeConnectEngine( QObject* parent, const QVariantList& args );
    ~KdeConnectEngine();

    virtual void init();

    /**
     *  This function implements part of Notifications DBus interface.
     *  Once called, will add notification source to the engine
     */
public slots:
     void notify(int solidError, const QString& error, const QString& errorDetails, const QString &udi);

private:
    uint m_id;
};

#endif
