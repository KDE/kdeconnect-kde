/**
 * SPDX-FileCopyrightText: 2014 Samoilenko Yuri <kinnalru@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KFilePlacesModel>
#include <core/device.h>
#include <core/kdeconnectplugin.h>

#define PACKET_TYPE_SFTP_REQUEST QStringLiteral("kdeconnect.sftp.request")
class Mounter;

class SftpPlugin : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.sftp")

public:
    explicit SftpPlugin(QObject *parent, const QVariantList &args);
    ~SftpPlugin() override;

    void receivePacket(const NetworkPacket &np) override;
    QString dbusPath() const override
    {
        return QLatin1String("/modules/kdeconnect/devices/%1/sftp").arg(deviceId);
    }

    Q_SCRIPTABLE bool startBrowsing();
    Q_SCRIPTABLE void mount();
    Q_SCRIPTABLE void unmount();
    Q_SCRIPTABLE bool mountAndWait();
    Q_SCRIPTABLE bool isMounted() const;
    Q_SCRIPTABLE QString getMountError();
    Q_SCRIPTABLE QString mountPoint();
    Q_SCRIPTABLE QVariantMap getDirectories(); // Actually a QMap<String, String>, but QDBus prefers this

Q_SIGNALS:
    Q_SCRIPTABLE void mounted();
    Q_SCRIPTABLE void unmounted();

private Q_SLOTS:
    void onMounted();
    void onUnmounted();
    void onFailed(const QString &message);

private:
    void knotify(int type, const QString &text, const QPixmap &icon) const;
    void addToDolphin();
    void removeFromDolphin();
    // Add KIO entry to Dolphin's Places
    KFilePlacesModel m_placesModel;
    Mounter *m_mounter;
    QString deviceId; // Storing it to avoid accessing device() from the destructor which could cause a crash

    QVariantMap remoteDirectories; // Actually a QMap<String, String>, but QDBus prefers this
    QString mountError;
};
