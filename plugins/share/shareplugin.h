/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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

#ifndef SHAREPLUGIN_H
#define SHAREPLUGIN_H

#include <QPointer>

#include <KIO/Job>

#include <core/kdeconnectplugin.h>
#include <core/compositefiletransferjob.h>

#define PACKET_TYPE_SHARE_REQUEST QStringLiteral("kdeconnect.share.request")
#define PACKET_TYPE_SHARE_REQUEST_UPDATE QStringLiteral("kdeconnect.share.request.update")

class SharePlugin
    : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.share")

public:
    explicit SharePlugin(QObject* parent, const QVariantList& args);

    ///Helper method, QDBus won't recognize QUrl
    Q_SCRIPTABLE void shareUrl(const QString& url) { shareUrl(QUrl(url)); }
    Q_SCRIPTABLE void shareUrls(const QStringList& urls);
    Q_SCRIPTABLE void shareText(const QString& text);
    Q_SCRIPTABLE void openFile(const QString& file) { openFile(QUrl(file)); }

    bool receivePacket(const NetworkPacket& np) override;
    void connected() override {}
    QString dbusPath() const override;

private Q_SLOTS:
    void openDestinationFolder();

Q_SIGNALS:
    Q_SCRIPTABLE void shareReceived(const QString& url);

private:
    void finished(KJob* job, const qint64 dateModified);
    void shareUrl(const QUrl& url);
    void openFile(const QUrl& url);
    QUrl destinationDir() const;
    QUrl getFileDestination(const QString filename) const;
    void setDateModified(const QUrl& destination, const qint64 timestamp);

    QPointer<CompositeFileTransferJob> m_compositeJob;
};
#endif
