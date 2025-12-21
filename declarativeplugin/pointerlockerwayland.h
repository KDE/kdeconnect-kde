/*
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef POINTERLOCKERWAYLAND_H
#define POINTERLOCKERWAYLAND_H

#include "pointerlocker.h"

class PointerConstraints;
class RelativePointerManagerV1;
class RelativePointerV1;
class LockedPointer;
struct wl_pointer;

class PointerLockerWayland : public AbstractPointerLocker
{
    Q_OBJECT
public:
    PointerLockerWayland(QObject *parent = nullptr);
    ~PointerLockerWayland() override;

    void setLocked(bool locked) override;
    bool isLocked() const override
    {
        return m_isLocked;
    }
    bool isLockEffective() const override;
    bool isSupported() const override
    {
        return m_pointerConstraints;
    }

    void setWindow(QWindow *window) override;

private:
    void setupRegistry();
    void enforceLock();
    void cleanupLock();

    wl_pointer *getPointer();

    bool m_isLocked = false;

    PointerConstraints *m_pointerConstraints;
    LockedPointer *m_lockedPointer = nullptr;
    std::unique_ptr<RelativePointerManagerV1> m_relativePointerMgr;
    std::unique_ptr<RelativePointerV1> m_relativePointer;
};

#endif
