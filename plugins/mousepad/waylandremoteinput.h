/**
 * SPDX-FileCopyrightText: 2018 Albert Vaca Cintora <albertvaka@gmail.com>
 * SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "abstractremoteinput.h"
#include "generated/systeminterfaces/remotedesktop.h"
#include <QDBusObjectPath>

class FakeInput;

class RemoteDesktopSession : public QObject
{
    Q_OBJECT
public:
    RemoteDesktopSession();
    void createSession();
    bool isValid() const
    {
        return m_connecting || !m_xdpPath.path().isEmpty();
    }
    OrgFreedesktopPortalRemoteDesktopInterface *const iface;
    QDBusObjectPath m_xdpPath;
    bool m_connecting = false;

private Q_SLOTS:
    void handleXdpSessionCreated(uint code, const QVariantMap &results);
    void handleXdpSessionConfigured(uint code, const QVariantMap &results);
    void handleXdpSessionStarted(uint code, const QVariantMap &results);
    void handleXdpSessionFinished(uint code, const QVariantMap &results);

private:
};

class WaylandRemoteInput : public AbstractRemoteInput
{
    Q_OBJECT

public:
    explicit WaylandRemoteInput(QObject *parent);

    bool handlePacket(const NetworkPacket &np) override;
    bool hasKeyboardSupport() override
    {
        return true;
    }
};
