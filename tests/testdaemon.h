/**
 * SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef TESTDAEMON_H
#define TESTDAEMON_H

#include <core/backends/pairinghandler.h>
#include <core/daemon.h>

#include <KJobTrackerInterface>
#include <QCoreApplication>

class TestDaemon : public Daemon
{
public:
    TestDaemon(QObject *parent = nullptr)
        : Daemon(parent, true)
        , m_jobTrackerInterface(nullptr)
    {
        // Necessary to force the event loop to run because the test harness seems to behave differently
        // and we need the QTimer::SingleShot in Daemon's constructor to fire
        QCoreApplication::processEvents();
    }

    void addDevice(Device *device)
    {
        Daemon::addDevice(device);
    }

    void reportError(const QString &title, const QString &description) override
    {
        qWarning() << "error:" << title << description;
    }

    void askPairingConfirmation(Device *d) override
    {
        d->acceptPairing();
    }

    Q_SCRIPTABLE virtual void sendSimpleNotification(const QString &eventId, const QString &title, const QString &text, const QString &iconName) override
    {
        qDebug() << eventId << title << text << iconName;
    }

    void quit() override
    {
        qDebug() << "quit was called";
    }

    KJobTrackerInterface *jobTracker() override
    {
        if (!m_jobTrackerInterface) {
            m_jobTrackerInterface = new KJobTrackerInterface();
        }
        return m_jobTrackerInterface;
    }

private:
    KJobTrackerInterface *m_jobTrackerInterface;
};

#endif
