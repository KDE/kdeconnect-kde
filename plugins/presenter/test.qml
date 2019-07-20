import QtQuick 2.10
import QtQuick.Layouts 1.10

Item
{
    width: 500
    height: 500

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
    }

    RowLayout {
        id: layout
        anchors.fill: parent
        spacing: 0
        Repeater {
            id: rep
            model: [ "white", "yellow", "red", "blue", "gray", "black" ]
            delegate: Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: modelData
            }
        }
    }

    Presenter {
        anchors.fill: parent

        xPos: mouse.mouseX/width
        yPos: mouse.mouseY/height
    }
}
