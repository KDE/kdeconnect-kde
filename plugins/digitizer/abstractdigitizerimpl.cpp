/**
 * SPDX-FileCopyrightText: 2025 Martin Sh <hemisputnik@proton.me>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "abstractdigitizerimpl.h"
#include <QObject>

AbstractDigitizerImpl::AbstractDigitizerImpl(QObject *parent, const Device *device)
    : QObject(parent)
    , m_device(device)
{
}

const Device *AbstractDigitizerImpl::device() const
{
    return m_device;
}

#include "moc_abstractdigitizerimpl.cpp"
