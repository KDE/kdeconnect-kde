/*
    SPDX-FileCopyrightText: 2025 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "keylistener.h"

#include <QInputMethodEvent>
#include <QJsonObject>
#include <QKeyEvent>

using namespace Qt::Literals::StringLiterals;

KeyListener::KeyListener(QObject *parent)
    : QObject(parent)
{
}

KeyListener::~KeyListener() = default;

bool KeyListener::eventFilter(QObject * /*watched*/, QEvent *event)
{
    if (event->type() == QEvent::KeyRelease) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (!keyEvent->text().isEmpty()) {
            return false;
        }
        const QVariantMap ppp = {
            {QStringLiteral("key"), keyEvent->key()},
            {QStringLiteral("text"), keyEvent->text()},
            {QStringLiteral("modifiers"), QVariant(keyEvent->modifiers())},
            {QStringLiteral("isAutoRepeat"), keyEvent->isAutoRepeat()},
            {QStringLiteral("count"), keyEvent->count()},
            {QStringLiteral("nativeScanCode"), keyEvent->nativeScanCode()},
            {QStringLiteral("accepted"), keyEvent->isAccepted()},
        };
        qDebug() << "sending..." << ppp;
        Q_EMIT keyReleased(ppp);
    } else if (event->type() == QEvent::InputMethod) {
        QInputMethodEvent *keyEvent = static_cast<QInputMethodEvent *>(event);
        qDebug() << "QInputMethodEvent!!!!!..." << keyEvent;
        Q_EMIT keyReleased({
            {QStringLiteral("text"), keyEvent->commitString()},
            {QStringLiteral("key"), -1},
        });
    } else {
        qDebug() << "GOT EVENT" << event->type() << event;
    }
    return false;
}

void KeyListener::setTarget(QObject *target)
{
    if (target == m_target) {
        return;
    }

    if (m_target) {
        m_target->removeEventFilter(this);
    }

    m_target = target;

    if (m_target) {
        m_target->installEventFilter(this);
    }
}
