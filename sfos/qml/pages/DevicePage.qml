/*
  Copyright (C) 2013 Jolla Ltd.
  Contact: Thomas Perl <thomas.perl@jollamobile.com>
  All rights reserved.

  You may use this file under the terms of BSD license as follows:

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Jolla Ltd nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

import QtQuick 2.0
import Sailfish.Silica 1.0
import org.kde.kdeconnect 1.0

Page {
    id: deviceView
    property QtObject currentDevice

    // The effective value will be restricted by ApplicationWindow.allowedOrientations
    allowedOrientations: Orientation.Portrait

    Column {
        id: column
        anchors.fill: parent
        spacing: Theme.paddingMedium

        PageHeader {
            title: currentDevice.name
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Device is " + (currentDevice.isTrusted ? "trusted" : "not trusted")
        }

        Button {
            id: text
            anchors.horizontalCenter: parent.horizontalCenter
            text: currentDevice.isTrusted ? "Un-Pair" : "Pair"

            onClicked: {
                if (currentDevice.isTrusted) {
                    currentDevice.unpair()
                } else {
                    currentDevice.requestPair()
                }
            }
        }

        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: currentDevice.isTrusted
            text: qsTr("Ping")
            onClicked: {
                currentDevice.pluginCall("ping", "sendPing");
            }
        }

        PluginItem {
            anchors.horizontalCenter: parent.horizontalCenter
            text: ("Multimedia control")
            interfaceFactory: MprisDbusInterfaceFactory
            component: "mpris.qml"
            pluginName: "mprisremote"
        }
        PluginItem {
            anchors.horizontalCenter: parent.horizontalCenter
            text: ("Remote input")
            interfaceFactory: RemoteControlDbusInterfaceFactory
            component: "mousepad.qml"
            pluginName: "remotecontrol"
        }
        PluginItem {
            anchors.horizontalCenter: parent.horizontalCenter
            readonly property var lockIface: LockDeviceDbusInterfaceFactory.create(deviceView.currentDevice.id())
            pluginName: "lockdevice"
            text: lockIface.isLocked ? ("Unlock") : ("Lock")
            onClicked: {
                lockIface.isLocked = !lockIface.isLocked;
            }
        }
    }
}


