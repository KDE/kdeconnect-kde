/*
    SPDX-FileCopyrightText: 2025 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QObject>
#include <QVariantMap>
#include <QWindow>
#include <qqmlintegration.h>

class KeyListener : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject *target MEMBER m_target WRITE setTarget)
public:
    KeyListener(QObject *parent = nullptr);
    ~KeyListener() override;

    void setTarget(QObject *target);

Q_SIGNALS:
    void keyReleased(const QVariantMap &event);

private:
    bool eventFilter(QObject *watched, QEvent *event) override;

    QObject *m_target = nullptr;
};
