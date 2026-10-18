// Fondo de escritorio (limpio: sin iconos). Clic derecho: menú del escritorio.
import QtQuick
import "."

Rectangle {
    color: Th.pal.bg
    Image {
        anchors.fill: parent
        source: Th.wallpaper ? "file://" + Th.wallpaper : ""
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        cache: false
        sourceSize: Qt.size(parent.width, parent.height)
    }
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton | Qt.LeftButton
        onClicked: function (m) {
            if (m.button === Qt.RightButton) Ux.toggle("desktopmenu", m.x, m.y)
            else Ux.close()
        }
    }
}
