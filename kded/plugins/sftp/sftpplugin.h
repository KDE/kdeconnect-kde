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

#ifndef SFTPPLUGIN_H
#define SFTPPLUGIN_H

#include <KProcess>
#include <QEventLoop>

#include "../kdeconnectplugin.h"

#define PACKAGE_TYPE_SFTP QLatin1String("kdeconnect.sftp")

class KNotification;

class SftpPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.sftp")
    
public:
  
    explicit SftpPlugin(QObject *parent, const QVariantList &args);
    virtual ~SftpPlugin();

    inline static KComponentData componentData() 
    {
        return KComponentData("kdeconnect", "kdeconnect");
    }

Q_SIGNALS:
  
    void mount_succesed(); //TODO make private - for internal use
    void mount_failed();  //TODO make private -for internal use
   
    
public Q_SLOTS:
    virtual bool receivePackage(const NetworkPackage& np);
    virtual void connected();

    Q_SCRIPTABLE void mount();
    Q_SCRIPTABLE void umount();
    Q_SCRIPTABLE bool mountAndWait();
    
    Q_SCRIPTABLE void startBrowsing();

    Q_SCRIPTABLE QString mountPoint();


protected:
     void timerEvent(QTimerEvent *event);
    
private Q_SLOTS:
    void onStarted();
    void onError(QProcess::ProcessError error);
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void mountTimeout();
    
    void readProcessOut();
    
    bool waitForMount();
    
private:
    QString dbusPath() const { return "/modules/kdeconnect/devices/" + device()->id() + "/sftp"; }
    void knotify(int type, const QString& text, const QPixmap& icon) const;
    void cleanMountPoint();
    void addToDolphin();
    void removeFromDolphin();
    
private:
    struct Pimpl;
    QScopedPointer<Pimpl> m_d;
};


#endif
