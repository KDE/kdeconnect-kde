/**
 * Copyright 2018 Adam Pigg <adam@piggz.co.uk>
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

#include <QCoreApplication>
#include <QNetworkAccessManager>

#include <KDBusService>
#include <KLocalizedString>

#include "core/daemon.h"
#include "core/device.h"
#include "core/backends/pairinghandler.h"
#include "kdeconnect-version.h"

#include <notification.h>
#include <backgroundactivity.h>
#include <QFile>
#include <QTextStream>

class SailfishDaemon : public Daemon
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.daemon")
public:
    SailfishDaemon(QObject* parent = nullptr)
        : Daemon(parent)
        , m_background(new BackgroundActivity())
    {
        connect(m_background, &BackgroundActivity::running,
            [=]( ) { qDebug() << "Received wakeup";
                m_background->wait(BackgroundActivity::ThirtySeconds);
            } );
        m_background->wait(BackgroundActivity::ThirtySeconds);
    }

    void askPairingConfirmation(Device* device) override
    {
        qDebug() << "Pairing request from " << device->name().toHtmlEscaped();

        Notification *notification = new Notification(this);

        notification->setAppName(QCoreApplication::applicationName());
        notification->setPreviewSummary(i18n("Pairing request from %1", device->name().toHtmlEscaped()));
        notification->setPreviewBody(i18n("Click here to pair"));
        notification->setIcon("icon-s-sync");
        notification->setExpireTimeout(10000);

        connect(notification, &Notification::closed,
                [=]( uint reason ) {
                    qDebug() << "Notification closed" << reason;
                    if (reason == 2) { //clicked
                        device->acceptPairing();
                    } else {
                        device->rejectPairing();
                    }
                });

        notification->publish();
    }

    void reportError(const QString & title, const QString & description) override
    {
        qDebug() << "Error:  " << title << ":" << description;
    }

    QNetworkAccessManager* networkAccessManager() override
    {
        if (!m_nam) {
            m_nam = new QNetworkAccessManager(this);
        }
        return m_nam;
    }

       void sendSimpleNotification(const QString &eventId, const QString &title, const QString &text, const QString &iconName) override
    {
        Q_UNUSED(eventId);
        Notification *notification = new Notification(this);

        notification->setAppName(QCoreApplication::applicationName());
        notification->setPreviewSummary(title);
        notification->setPreviewBody(text);
        notification->setIcon(iconName);
        notification->publish();
    }


private:
    QNetworkAccessManager* m_nam = nullptr;
    BackgroundActivity *m_background = nullptr;

};


void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();

    QString txt;
    QString typ;

    switch (type) {
    case QtDebugMsg:
        typ = "Debug";
        break;
    case QtInfoMsg:
        typ = "Info";
        break;
    case QtWarningMsg:
        typ = "Warning";
        break;
    case QtCriticalMsg:
        typ = "Critical";
        break;
    case QtFatalMsg:
        break;
    }

    txt = QString("%1 %2: %3 (%4:%5, %6)").arg(QDateTime::currentDateTime().toString("yyyyMMdd:HHmmss")).arg(typ).arg(localMsg.constData()).arg(context.file).arg(context.line).arg(context.function);

    QFile outFile("/home/nemo/kdeconnectd.log");
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    ts << txt << endl;
}

int main(int argc, char* argv[])
{
    qInstallMessageHandler(myMessageOutput); // Install the handler
    QCoreApplication app(argc, argv);

    app.setApplicationName(QStringLiteral("kdeconnectd"));
    app.setApplicationVersion(QStringLiteral(KDECONNECT_VERSION_STRING));
    app.setOrganizationDomain(QStringLiteral("kde.org"));

    KDBusService dbusService(KDBusService::Unique);

    Daemon* daemon = new SailfishDaemon;

    QObject::connect(daemon, SIGNAL(destroyed(QObject*)), &app, SLOT(quit()));

    return app.exec();
}

#include "sailfishdaemon.moc"
