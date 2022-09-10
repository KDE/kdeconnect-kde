/**
 * SPDX-FileCopyrightText: 2014 Samoilenko Yuri <kinnalru@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef SFTPPLUGIN_H
#define SFTPPLUGIN_H

#include <core/device.h>
#include <core/kdeconnectplugin.h>

#define PACKET_TYPE_SFTP_REQUEST QStringLiteral("kdeconnect.sftp.request")

class SftpPlugin : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.sftp")

public:
    explicit SftpPlugin(QObject *parent, const QVariantList &args);
    ~SftpPlugin() override;

    bool receivePacket(const NetworkPacket &np) override;
    void connected() override
    {
    }
    QString dbusPath() const override
    {
        return QStringLiteral("/modules/kdeconnect/devices/") + deviceId + QStringLiteral("/sftp");
    }

Q_SIGNALS:
    void packetReceived(const NetworkPacket &np);
    Q_SCRIPTABLE void mounted();
    Q_SCRIPTABLE void unmounted();

public Q_SLOTS:
    Q_SCRIPTABLE void mount();
    Q_SCRIPTABLE void unmount();
    Q_SCRIPTABLE bool mountAndWait();
    Q_SCRIPTABLE bool isMounted() const;
    Q_SCRIPTABLE QString getMountError();

    Q_SCRIPTABLE bool startBrowsing();
    Q_SCRIPTABLE QString mountPoint();
    Q_SCRIPTABLE QVariantMap getDirectories(); // Actually a QMap<String, String>, but QDBus prefers this

private Q_SLOTS:
    void onMounted();
    void onUnmounted();
    void onFailed(const QString &message);

private:
    void knotify(int type, const QString &text, const QPixmap &icon) const;
    void addToDolphin();
    void removeFromDolphin();

private:
    struct Pimpl;
    QScopedPointer<Pimpl> d;
    QString deviceId; // Storing it to avoid accessing device() from the destructor which could cause a crash

    QVariantMap remoteDirectories; // Actually a QMap<String, String>, but QDBus prefers this
    QString mountError;
};

#endif
