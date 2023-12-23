/**
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls

Loader {

    property var plugin
    property var lastPosition: plugin.position
    property date lastPositionTime: new Date()
    property bool updatePositionSlider: true

    sourceComponent: plugin.canSeek ? seekBar : progressBar

    onLastPositionChanged: {
        if (item != null) {
            item.value = lastPosition
            lastPositionTime = new Date();
        }
    }

    Component {
        id: seekBar

        Slider {
            from: 0
            to: plugin.length
            onPressedChanged: {
                if (pressed) {
                    updatePositionSlider = false
                } else {
                    updatePositionSlider = true
                    plugin.position = value
                }
            }
        }
    }

    Component {
        id: progressBar

        ProgressBar {
            from: 0
            to: plugin.length
        }
    }

    Timer {
        id: positionUpdateTimer
        interval: 1000
        repeat: true
        running: updatePositionSlider && plugin.isPlaying

        onTriggered: item.value = lastPosition + (new Date().getTime() - lastPositionTime.getTime())
    }
}
