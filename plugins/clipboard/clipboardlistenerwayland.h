/**
 * Copyright 2019 Aleix Pol Gonzalez
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

#ifndef CLIPBOARDLISTENERWAYLAND_H
#define CLIPBOARDLISTENERWAYLAND_H

#include "clipboardlistener.h"
#include <KWayland/Client/datacontroldevicemanager.h>
#include <KWayland/Client/datacontrolsource.h>
#include <KWayland/Client/datacontroloffer.h>
#include <KWayland/Client/connection_thread.h>

class ClipboardListenerWayland : public ClipboardListener
{
    Q_OBJECT
public:
    ClipboardListenerWayland();

    void setText(const QString & content) override;

private:
    void sendData(const QString &mimeType, qint32 fd);
    void selectionReceived(KWayland::Client::DataControlOffer* offer);
    QString readString(const QMimeType& mimeType, KWayland::Client::DataControlOffer* offer);

    QString m_currentContent;
    KWayland::Client::ConnectionThread* m_waylandConnection = nullptr;
    KWayland::Client::DataControlDeviceManager* m_manager = nullptr;
    KWayland::Client::DataControlSource* m_source = nullptr;
    KWayland::Client::Seat* m_seat = nullptr;
    KWayland::Client::DataControlDevice* m_dataControlDevice = nullptr;
};

#endif
