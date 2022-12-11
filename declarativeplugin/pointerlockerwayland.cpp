/*
    SPDX-FileCopyrightText: 2018 Roman Gilg <subdiff@gmail.com>
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "pointerlockerwayland.h"

#include <QDebug>

#include "qwayland-pointer-constraints-unstable-v1.h"
#include <QtWaylandClient/qwaylandclientextension.h>
#include <qpa/qplatformnativeinterface.h>

#include <QGuiApplication>

class PointerConstraints : public QWaylandClientExtensionTemplate<PointerConstraints>, public QtWayland::zwp_pointer_constraints_v1
{
public:
    PointerConstraints()
        : QWaylandClientExtensionTemplate<PointerConstraints>(1)
    {
    }
};

class LockedPointer : public QObject, public QtWayland::zwp_locked_pointer_v1
{
    Q_OBJECT
public:
    LockedPointer(struct ::zwp_locked_pointer_v1 *object, QObject *parent)
        : QObject(parent)
        , zwp_locked_pointer_v1(object)
    {
    }

    Q_SIGNAL void locked();

    Q_SIGNAL void unlocked();

private:
    void zwp_locked_pointer_v1_locked() override
    {
        Q_EMIT locked();
    }

    void zwp_locked_pointer_v1_unlocked() override
    {
        Q_EMIT unlocked();
    }
};

PointerLockerWayland::PointerLockerWayland(QObject *parent)
    : AbstractPointerLocker(parent)
{
    m_pointerConstraints = new PointerConstraints;
}

PointerLockerWayland::~PointerLockerWayland()
{
    delete m_pointerConstraints;
}

bool PointerLockerWayland::isLockEffective() const
{
    return m_lockedPointer;
}

wl_pointer *PointerLockerWayland::getPointer()
{
    QPlatformNativeInterface *native = qGuiApp->platformNativeInterface();
    if (!native) {
        return nullptr;
    }

    window()->create();

    return reinterpret_cast<wl_pointer *>(native->nativeResourceForIntegration(QByteArrayLiteral("wl_pointer")));
}

void PointerLockerWayland::enforceLock()
{
    if (!m_isLocked) {
        return;
    }

    wl_surface *wlSurface = [](QWindow *window) -> wl_surface * {
        if (!window) {
            return nullptr;
        }

        QPlatformNativeInterface *native = qGuiApp->platformNativeInterface();
        if (!native) {
            return nullptr;
        }
        window->create();
        return reinterpret_cast<wl_surface *>(native->nativeResourceForWindow(QByteArrayLiteral("surface"), window));
    }(m_window);

    m_lockedPointer =
        new LockedPointer(m_pointerConstraints->lock_pointer(wlSurface, getPointer(), nullptr, PointerConstraints::lifetime::lifetime_persistent), this);

    if (!m_lockedPointer) {
        qDebug() << "ERROR when receiving locked pointer!";
        return;
    }

    connect(m_lockedPointer, &LockedPointer::locked, this, [this] {
        Q_EMIT lockEffectiveChanged(true);
    });
    connect(m_lockedPointer, &LockedPointer::unlocked, this, [this] {
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
    m_lockedPointer->destroy();
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

#include "pointerlockerwayland.moc"
