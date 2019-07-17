import QtQuick 2.2
import QtQuick.Controls 2.2

Loader {

    property var plugin
    property var lastPosition: plugin.position
    property date lastPositionTime: new Date()
    property bool updatePositionSlider: true

    sourceComponent: plugin.canSeek ? seekBar : progressBar

    onLastPositionChanged: {
        lastPositionTime = new Date();
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

    Connections {
        target: plugin
        onNowPlayingChanged: {
            item.value = lastPosition
        }
    }
}
