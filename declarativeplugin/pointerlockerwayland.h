/*
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef POINTERLOCKERWAYLAND_H
#define POINTERLOCKERWAYLAND_H

#include "pointerlocker.h"

namespace KWayland {
namespace Client {

class ConnectionThread;
class Registry;
class Compositor;
class Seat;
class Pointer;
class PointerConstraints;
class LockedPointer;
class ConfinedPointer;
class RelativePointer;
class RelativePointerManager;

}
}

class PointerLockerWayland : public AbstractPointerLocker
{
    Q_OBJECT
public:
    PointerLockerWayland(QObject *parent = nullptr);

    void setLocked(bool locked) override;
    bool isLocked() const override { return m_isLocked; }
    bool isLockEffective() const override;
    bool isSupported() const override {
        return m_pointerConstraints && m_relativePointerManager;
    }

    void setWindow(QWindow * window) override;

private:
    void setupRegistry();
    void enforceLock();
    void cleanupLock();

    bool m_isLocked = false;
    KWayland::Client::ConnectionThread *m_connectionThreadObject;
    KWayland::Client::Compositor *m_compositor = nullptr;
    KWayland::Client::Seat *m_seat = nullptr;
    KWayland::Client::Pointer *m_pointer = nullptr;
    KWayland::Client::PointerConstraints *m_pointerConstraints = nullptr;
    KWayland::Client::RelativePointer *m_relativePointer = nullptr;
    KWayland::Client::RelativePointerManager *m_relativePointerManager = nullptr;

    KWayland::Client::LockedPointer *m_lockedPointer = nullptr;
};

#endif
