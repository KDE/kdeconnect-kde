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

#include "pausemusicpackageinterface.h"

#include <QDebug>

PauseMusicPackageInterface::PauseMusicPackageInterface()
{
    //TODO: Be able to change this from settings
    pauseWhen = PauseWhenRinging;
    paused = false;

}

bool PauseMusicPackageInterface::receivePackage (const Device& device, const NetworkPackage& np)
{
    Q_UNUSED(device);

    bool pauseConditionFulfilled = false;

    //TODO: I have manually tested it and it works for both cases, but I should somehow write a test for this logic
    if (pauseWhen == PauseWhenRinging) {
        if (np.type() == PACKAGE_TYPE_NOTIFICATION) {
            if (np.get<QString>("notificationType") != "ringing") return false;
            pauseConditionFulfilled = !np.get<bool>("isCancel");
        } else if (np.type() == PACKAGE_TYPE_CALL) {
            pauseConditionFulfilled = !np.get<bool>("isCancel");
        } else {
            return false;
        }
    } else if (pauseWhen == PauseWhenTalking){
        if (np.type() != PACKAGE_TYPE_CALL) return false;
        pauseConditionFulfilled = !np.get<bool>("isCancel");
    }

    qDebug() << "PauseMusicPackageReceiver - PauseCondition:" << pauseConditionFulfilled;

    if (pauseConditionFulfilled && !paused) {
        //TODO: Use KDE DBUS API
        system("qdbus org.mpris.MediaPlayer2.spotify /org/mpris/MediaPlayer2 org.mpris.MediaPlayer2.Player.Pause");
    } if (!pauseConditionFulfilled && paused) {
        //FIXME: Play does not work, using PlayPause
        system("qdbus org.mpris.MediaPlayer2.spotify /org/mpris/MediaPlayer2 org.mpris.MediaPlayer2.Player.PlayPause");
    }

    paused = pauseConditionFulfilled;

    return true;

}
