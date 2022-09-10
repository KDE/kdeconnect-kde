/*
 * SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef DEVICEINDICATOR_H
#define DEVICEINDICATOR_H

#include <QMenu>

class DeviceDbusInterface;
class RemoteCommandsDbusInterface;

class DeviceIndicator : public QMenu
{
    Q_OBJECT
public:
    DeviceIndicator(DeviceDbusInterface *device);

public Q_SLOTS:
    void setText(const QString &text)
    {
        setTitle(text);
    }

private:
    DeviceDbusInterface *m_device;
    RemoteCommandsDbusInterface *m_remoteCommandsInterface;
};

#endif // DEVICEINDICATOR_H
