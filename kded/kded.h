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

#ifndef KDECONNECT_KDED_H
#define KDECONNECT_KDED_H

#include <KDEDModule>
#include <KProcess>

class Kded
    : public KDEDModule
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kded.kdeconnect")

public:
    Kded(QObject *parent, const QList<QVariant>&);
    ~Kded();

public Q_SLOTS:
    
    Q_SCRIPTABLE bool start();
    Q_SCRIPTABLE void stop();
    Q_SCRIPTABLE bool restart();

private Q_SLOTS:
    void onError(QProcess::ProcessError);
    void onFinished(int, QProcess::ExitStatus);
    
private:
    KProcess* m_daemon;
};

#endif
