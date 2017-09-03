/**
 * Copyright 2014 Samoilenko Yuri<kinnalru@gmail.com>
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

#ifndef SFTPPLUGIN_H
#define SFTPPLUGIN_H

#include <core/kdeconnectplugin.h>
#include <core/device.h>

#define PACKAGE_TYPE_SFTP_REQUEST QStringLiteral("kdeconnect.sftp.request")

class KNotification;

class SftpPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.sftp")
    
public:
    explicit SftpPlugin(QObject* parent, const QVariantList& args);
    ~SftpPlugin() override;

    bool receivePackage(const NetworkPackage& np) override;
    void connected() override {}
    QString dbusPath() const override { return "/modules/kdeconnect/devices/" + deviceId + "/sftp"; }

Q_SIGNALS:
    void packageReceived(const NetworkPackage& np);
    Q_SCRIPTABLE void mounted();
    Q_SCRIPTABLE void unmounted();

public Q_SLOTS:
    Q_SCRIPTABLE void mount();
    Q_SCRIPTABLE void unmount();
    Q_SCRIPTABLE bool mountAndWait();
    Q_SCRIPTABLE bool isMounted() const;

    Q_SCRIPTABLE bool startBrowsing();
    Q_SCRIPTABLE QString mountPoint();
    Q_SCRIPTABLE QVariantMap getDirectories(); //Actually a QMap<String, String>, but QDBus preffers this

private Q_SLOTS:
    void onMounted();
    void onUnmounted();
    void onFailed(const QString& message);
    
private:
    void knotify(int type, const QString& text, const QPixmap& icon) const;
    void addToDolphin();
    void removeFromDolphin();
    
private:
    struct Pimpl;
    QScopedPointer<Pimpl> d;
    QString deviceId; //Storing it to avoid accessing device() from the destructor which could cause a crash

    QVariantMap remoteDirectories; //Actually a QMap<String, String>, but QDBus preffers this
};


#endif
