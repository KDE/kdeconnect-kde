/*
    SPDX-FileCopyrightText: 2018 Roman Gilg <subdiff@gmail.com>
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "pointerlocker.h"

#include <QCursor>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>

#include <QDebug>

void AbstractPointerLocker::setWindow(QWindow *window)
{
    if (m_window == window) {
        return;
    }
    m_window = window;
    Q_EMIT windowChanged();
}

PointerLockerQt::PointerLockerQt(QObject *parent)
    : AbstractPointerLocker(parent)
{
}

PointerLockerQt::~PointerLockerQt() = default;

void PointerLockerQt::setLocked(bool lock)
{
    if (m_isLocked == lock) {
        return;
    }
    m_isLocked = lock;

    if (lock) {
        /* Cursor needs to be hidden such that Xwayland emulates warps. */
        QGuiApplication::setOverrideCursor(QCursor(Qt::BlankCursor));
        m_originalPosition = QCursor::pos();
        m_window->installEventFilter(this);
        Q_EMIT lockedChanged(true);
        Q_EMIT lockEffectiveChanged(true);
    } else {
        m_window->removeEventFilter(this);
        QGuiApplication::restoreOverrideCursor();
        Q_EMIT lockedChanged(false);
        Q_EMIT lockEffectiveChanged(false);
    }
}

bool PointerLockerQt::isLocked() const
{
    return m_isLocked;
}

bool PointerLockerQt::eventFilter(QObject *watched, QEvent *event)
{
    if (watched != m_window || event->type() != QEvent::MouseMove || !isLocked()) {
        return false;
    }

    const auto newPos = QCursor::pos();
    const QPointF dist = newPos - m_originalPosition;
    Q_EMIT pointerMoved({dist.x(), dist.y()});
    QCursor::setPos(m_originalPosition);

    return true;
}

#include "moc_pointerlocker.cpp"
