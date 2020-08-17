/**
 * SPDX-FileCopyrightText: 2018 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef WAYLANDREMOTEINPUT_H
#define WAYLANDREMOTEINPUT_H

#include <QPointer>
#include "abstractremoteinput.h"

namespace KWayland
{
    namespace Client
    {
        class FakeInput;
    }
}

class WaylandRemoteInput
    : public AbstractRemoteInput
{
    Q_OBJECT

public:
    explicit WaylandRemoteInput(QObject* parent);

    bool handlePacket(const NetworkPacket& np) override;

private:
    void setupWaylandIntegration();

    QPointer<KWayland::Client::FakeInput> m_waylandInput;
    bool m_waylandAuthenticationRequested;
};

#endif
