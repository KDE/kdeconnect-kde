/**
 * SPDX-FileCopyrightText: 2018 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef WAYLANDREMOTEINPUT_H
#define WAYLANDREMOTEINPUT_H

#include "abstractremoteinput.h"

class FakeInput;
class WlKeyboard;

class WaylandRemoteInput
    : public AbstractRemoteInput
{
    Q_OBJECT

public:
    explicit WaylandRemoteInput(QObject* parent);
    ~WaylandRemoteInput();

    void setKeymap(const QByteArray &keymap) {
        m_keymapSent = false;
        m_keymap = keymap;
    }
    bool handlePacket(const NetworkPacket& np) override;

private:
    void setupWaylandIntegration();

    QScopedPointer<FakeInput> m_fakeInput;
    QScopedPointer<WlKeyboard> m_keyboard;
    bool m_waylandAuthenticationRequested;
    bool m_keymapSent = false;
    QByteArray m_keymap;
};

#endif
