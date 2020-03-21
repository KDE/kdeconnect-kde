/**
 * Copyright 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
            m_nam = new KIO::AccessManager(this);
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
