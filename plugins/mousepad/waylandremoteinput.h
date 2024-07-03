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
#include <QSocketNotifier>

class FakeInput;
class Xkb;
struct ei;
struct ei_device;

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

    void pointerButton(int button, bool press);
    void pointerAxis(double dx, double dy);
    void pointerMotion(double dx, double dy);
    void pointerMotionAbsolute(double x, double y);
    void keyboardKeysym(int sym, bool press);
    void keyboardKeycode(int key, bool press);

private Q_SLOTS:
    void handleXdpSessionCreated(uint code, const QVariantMap &results);
    void handleXdpSessionConfigured(uint code, const QVariantMap &results);
    void handleXdpSessionStarted(uint code, const QVariantMap &results);
    void handleXdpSessionFinished(uint code, const QVariantMap &results);

private:
    void connectToEi(int fd);
    void handleEiEvents();
    QSocketNotifier m_eiNotifier;
    std::unique_ptr<Xkb> m_xkb;
    ei *m_ei = nullptr;
    ei_device *m_keyboard = nullptr;
    ei_device *m_pointer = nullptr;
    ei_device *m_absolutePointer = nullptr;
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
