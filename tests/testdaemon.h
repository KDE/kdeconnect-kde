/**
 * SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef TESTDAEMON_H
#define TESTDAEMON_H

#include <core/daemon.h>
#include <core/backends/pairinghandler.h>

#include <QCoreApplication>
#include <KJobTrackerInterface>

#ifdef HAVE_KIO
#include <KIO/AccessManager>
#else
#include <QNetworkAccessManager>
#endif

class TestDaemon : public Daemon
{
public:
    TestDaemon(QObject* parent = nullptr)
        : Daemon(parent, true)
        , m_nam(nullptr)
        , m_jobTrackerInterface(nullptr)
    {
        // Necessary to force the event loop to run because the test harness seems to behave differently
        // and we need the QTimer::SingleShot in Daemon's constructor to fire
        QCoreApplication::processEvents();
    }

    void addDevice(Device* device) {
        Daemon::addDevice(device);
    }

    void reportError(const QString & title, const QString & description) override
    {
        qWarning() << "error:" << title << description;
    }

    void askPairingConfirmation(Device * d) override {
        d->acceptPairing();
    }

    QNetworkAccessManager* networkAccessManager() override
    {
        if (!m_nam) {
#ifdef HAVE_KIO
            m_nam = new KIO::Integration::AccessManager(this);
#else
            m_nam = new QNetworkAccessManager(this);
#endif
        }
        return m_nam;
    }

    Q_SCRIPTABLE virtual void sendSimpleNotification(const QString &eventId, const QString &title, const QString &text, const QString &iconName) override
    {
        qDebug() << eventId << title << text << iconName;
    }

    void quit() override {
        qDebug() << "quit was called";
    }

    KJobTrackerInterface* jobTracker() override
    {
        if (!m_jobTrackerInterface) {
            m_jobTrackerInterface = new KJobTrackerInterface();
        }
        return m_jobTrackerInterface;
    }

private:
    QNetworkAccessManager* m_nam;
     KJobTrackerInterface* m_jobTrackerInterface;
};

#endif
