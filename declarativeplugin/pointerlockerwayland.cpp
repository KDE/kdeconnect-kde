/*
    SPDX-FileCopyrightText: 2018 Roman Gilg <subdiff@gmail.com>
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "pointerlockerwayland.h"

#include <QDebug>

#include <KWayland/Client/compositor.h>
#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/pointer.h>
#include <KWayland/Client/pointerconstraints.h>
#include <KWayland/Client/region.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/relativepointer.h>
#include <KWayland/Client/seat.h>
#include <KWayland/Client/surface.h>

using namespace KWayland::Client;

PointerLockerWayland::PointerLockerWayland(QObject *parent)
    : AbstractPointerLocker(parent)
    , m_connectionThreadObject(ConnectionThread::fromApplication(this))
{
    setupRegistry();
}

void PointerLockerWayland::setupRegistry()
{
    Registry *registry = new Registry(this);

    connect(registry, &Registry::compositorAnnounced, this, [this, registry](quint32 name, quint32 version) {
        m_compositor = registry->createCompositor(name, version, this);
    });
    connect(registry, &Registry::relativePointerManagerUnstableV1Announced, this, [this, registry](quint32 name, quint32 version) {
        Q_ASSERT(!m_relativePointerManager);
        m_relativePointerManager = registry->createRelativePointerManager(name, version, this);
    });
    connect(registry, &Registry::seatAnnounced, this, [this, registry](quint32 name, quint32 version) {
        m_seat = registry->createSeat(name, version, this);
        if (m_seat->hasPointer()) {
            m_pointer = m_seat->createPointer(this);
        }
        connect(m_seat, &Seat::hasPointerChanged, this, [this](bool hasPointer) {
            delete m_pointer;

            if (!hasPointer)
                return;

            m_pointer = m_seat->createPointer(this);

            delete m_relativePointer;
            m_relativePointer = m_relativePointerManager->createRelativePointer(m_pointer, this);
            connect(m_relativePointer, &RelativePointer::relativeMotion, this, [this](const QSizeF &delta) {
                Q_EMIT pointerMoved({delta.width(), delta.height()});
            });
        });
    });
    connect(registry, &Registry::pointerConstraintsUnstableV1Announced, this, [this, registry](quint32 name, quint32 version) {
        m_pointerConstraints = registry->createPointerConstraints(name, version, this);
    });
    connect(registry, &Registry::interfacesAnnounced, this, [this] {
        Q_ASSERT(m_compositor);
        Q_ASSERT(m_seat);
        Q_ASSERT(m_pointerConstraints);
    });
    registry->create(m_connectionThreadObject);
    registry->setup();
}

bool PointerLockerWayland::isLockEffective() const
{
    return m_lockedPointer && m_lockedPointer->isValid();
}

void PointerLockerWayland::enforceLock()
{
    if (!m_isLocked) {
        return;
    }

    QScopedPointer<Surface> winSurface(Surface::fromWindow(m_window));
    if (!winSurface) {
        qWarning() << "Locking a window that is not mapped";
        return;
    }
    auto *lockedPointer = m_pointerConstraints->lockPointer(winSurface.data(), m_pointer, nullptr, PointerConstraints::LifeTime::Persistent, this);

    if (!lockedPointer) {
        qDebug() << "ERROR when receiving locked pointer!";
        return;
    }
    m_lockedPointer = lockedPointer;

    connect(lockedPointer, &LockedPointer::locked, this, [this] {
        Q_EMIT lockEffectiveChanged(true);
    });
    connect(lockedPointer, &LockedPointer::unlocked, this, [this] {
        Q_EMIT lockEffectiveChanged(false);
    });
}

void PointerLockerWayland::setLocked(bool lock)
{
    if (m_isLocked == lock) {
        return;
    }

    if (!isSupported()) {
        qWarning() << "Locking before having our interfaces announced";
        return;
    }

    m_isLocked = lock;
    if (lock) {
        enforceLock();
    } else {
        cleanupLock();
    }
    Q_EMIT lockedChanged(lock);
}

void PointerLockerWayland::cleanupLock()
{
    if (!m_lockedPointer) {
        return;
    }
    m_lockedPointer->release();
    m_lockedPointer->deleteLater();
    m_lockedPointer = nullptr;
    Q_EMIT lockEffectiveChanged(false);
}

void PointerLockerWayland::setWindow(QWindow *window)
{
    if (m_window == window) {
        return;
    }
    cleanupLock();

    if (m_window) {
        disconnect(m_window, &QWindow::visibleChanged, this, &PointerLockerWayland::enforceLock);
    }
    AbstractPointerLocker::setWindow(window);
    connect(m_window, &QWindow::visibleChanged, this, &PointerLockerWayland::enforceLock);

    if (m_isLocked) {
        enforceLock();
    }
}
