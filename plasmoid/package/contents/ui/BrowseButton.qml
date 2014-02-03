/**
 * Copyright 2014 Samoilenko Yuri <kinnalru@gmail.com>
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

import QtQuick 1.1
import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.plasma.components 0.1 as PlasmaComponents
import org.kde.kdeconnect 1.0

PlasmaComponents.Button
{
    id: button
    checkable: true
    property string deviceId: ""
    property variant sftp: SftpDbusInterfaceFactory.create(deviceId)

    state: "UNMOUNTED"
                
    states: [
        State {
            name: "UNMOUNTED"
            PropertyChanges { target: button; checked: false; text: "Browse" }
        },
        State {
            name: "MOUNTING"
            PropertyChanges { target: button; checked: true; text: "Mounting..." }
        },
        State {
            name: "MOUNTED"
            PropertyChanges { target: button; checked: false; text: "Unmount" }
        }
    ]
    
    onClicked: {
        if (state == "UNMOUNTED") {
            state = "MOUNTING"
            sftp.startBrowsing()
        }
        else if (state == "MOUNTED") {
            sftp.umount()
            state = "UNMOUNTED"
        }
    }
    

    DBusAsyncResponse {
        id: startupCheck
        onSuccess: {
            console.log("222SUCC:" + result)
            if (result) {
                button.state = "MOUNTED"
            }
            else {
                button.state = "UNMOUNTED"
            }
        }
        onError: {
            console.log("Error:" + message)
            state = "UNMOUNTED"
        }
    }
    
    Component.onCompleted: {
      
        console.log("onCompleted device:" + deviceId)
        
        sftp.mounted.connect(function() {
            browse.state = "MOUNTED"
        })
        
        sftp.unmounted.connect(function() {
            browse.state = "UNMOUNTED"
        })
        
        startupCheck.pendingCall = sftp.isMounted()
    }
    
}
