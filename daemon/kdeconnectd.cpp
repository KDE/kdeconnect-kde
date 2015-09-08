/**
 * Copyright 2014 Yuri Samoilenko <kinnalru@gmail.com>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>

#include <QSocketNotifier>
#include <QApplication>
#include <QNetworkAccessManager>

#include <KDBusService>
#include <KNotification>
#include <KLocalizedString>
#include <KIO/AccessManager>

#include "core/daemon.h"
#include "core/device.h"
#include "kdeconnect-version.h"

static int sigtermfd[2];
const static char deadbeef = 1;
struct sigaction action;

void sighandler(int signum)
{
    if( signum == SIGTERM || signum == SIGINT)
    {
        ssize_t unused = ::write(sigtermfd[0], &deadbeef, sizeof(deadbeef));
        Q_UNUSED(unused);
    }
}

void initializeTermHandlers(QCoreApplication* app, Daemon* daemon)
{
    ::socketpair(AF_UNIX, SOCK_STREAM, 0, sigtermfd);
    QSocketNotifier* snTerm = new QSocketNotifier(sigtermfd[1], QSocketNotifier::Read, app);
    QObject::connect(snTerm, SIGNAL(activated(int)), daemon, SLOT(deleteLater()));

    action.sa_handler = sighandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    sigaction(SIGTERM, &action, nullptr);
    sigaction(SIGINT, &action, nullptr);
}

class DesktopDaemon : public Daemon
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.daemon")
public:
    DesktopDaemon(QObject* parent = Q_NULLPTR)
        : Daemon(parent)
        , m_nam(Q_NULLPTR)
    {}

    void requestPairing(Device* d) Q_DECL_OVERRIDE
    {
        KNotification* notification = new KNotification("pairingRequest");
        notification->setIconName(QStringLiteral("dialog-information"));
        notification->setComponentName("kdeconnect");
        notification->setText(i18n("Pairing request from %1", d->name()));
        notification->setActions(QStringList() << i18n("Accept") << i18n("Reject"));
        connect(notification, &KNotification::ignored, d, &Device::rejectPairing);
        connect(notification, &KNotification::action1Activated, d, &Device::acceptPairing);
        connect(notification, &KNotification::action2Activated, d, &Device::rejectPairing);
        notification->sendEvent();
    }

    void reportError(const QString & title, const QString & description) Q_DECL_OVERRIDE
    {
        KNotification::event(KNotification::Error, title, description);
    }

    QNetworkAccessManager* networkAccessManager() Q_DECL_OVERRIDE
    {
        if (!m_nam) {
            m_nam = new KIO::AccessManager(this);
        }
        return m_nam;
    }

private:
    QNetworkAccessManager* m_nam;
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("kdeconnectd");
    app.setApplicationVersion(QLatin1String(KDECONNECT_VERSION_STRING));
    app.setOrganizationDomain("kde.org");
    app.setQuitOnLastWindowClosed(false);

    KDBusService dbusService(KDBusService::Unique);

    Daemon* daemon = new DesktopDaemon;
    QObject::connect(daemon, SIGNAL(destroyed(QObject*)), &app, SLOT(quit()));
    initializeTermHandlers(&app, daemon);

    return app.exec();
}

#include "kdeconnectd.moc"
