/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QPointer>

#include <core/compositefiletransferjob.h>
#include <core/kdeconnectplugin.h>

#define PACKET_TYPE_SHARE_REQUEST QStringLiteral("kdeconnect.share.request")
#define PACKET_TYPE_SHARE_REQUEST_UPDATE QStringLiteral("kdeconnect.share.request.update")

class SharePlugin : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.share")

public:
    explicit SharePlugin(QObject *parent, const QVariantList &args);

    /// Helper method, QDBus won't recognize QUrl
    Q_SCRIPTABLE void shareUrl(const QString &url)
    {
        shareUrl(QUrl(url), false);
    }
    Q_SCRIPTABLE void shareUrls(const QStringList &urls);
    Q_SCRIPTABLE void shareText(const QString &text);
    Q_SCRIPTABLE void openFile(const QString &file)
    {
        shareUrl(QUrl(file), true);
    }

    void receivePacket(const NetworkPacket &np) override;
    QString dbusPath() const override;

private Q_SLOTS:
    void openDestinationFolder();

Q_SIGNALS:
    Q_SCRIPTABLE void shareReceived(const QString &url);

private:
    void finished(KJob *job, const qint64 dateCreated, const qint64 dateModified, const bool open);
    void shareUrl(const QUrl &url, bool open);
    QUrl destinationDir() const;
    QUrl getFileDestination(const QString filename) const;
    void setDateModified(const QUrl &destination, const qint64 timestamp);
    void setDateCreated(const QUrl &destination, const qint64 timestamp);

    QPointer<CompositeFileTransferJob> m_compositeJob;
};
