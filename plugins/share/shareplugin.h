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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SHAREPLUGIN_H
#define SHAREPLUGIN_H

#include <KNotification>
#include <KIO/Job>

#include <core/kdeconnectplugin.h>

#define PACKAGE_TYPE_SHARE_REQUEST QStringLiteral("kdeconnect.share.request")

class SharePlugin
    : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.share")

public:
    explicit SharePlugin(QObject* parent, const QVariantList& args);

    ///Helper method, QDBus won't recognize QUrl
    Q_SCRIPTABLE void shareUrl(const QString& url) { shareUrl(QUrl(url)); }

    bool receivePackage(const NetworkPackage& np) override;
    void connected() override {}
    QString dbusPath() const override;

private Q_SLOTS:
    void finished(KJob*);
    void openDestinationFolder();

Q_SIGNALS:
    void shareReceived(const QString& url);

private:
    void shareUrl(const QUrl& url);

    QUrl destinationDir() const;

};
#endif
