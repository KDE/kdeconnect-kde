import QtQuick 2.10

Rectangle
{
    color: "red"
    width: 500
    height: 500

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
    }

    Presenter {
        anchors.fill: parent

        xPos: mouse.mouseX/width
        yPos: mouse.mouseY/height
    }
}
