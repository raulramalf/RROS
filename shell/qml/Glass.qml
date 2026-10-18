// Superficie de cristal de RROS: el color del tema translúcido, borde fino y un brillo arriba
import QtQuick
import "."

Rectangle {
    property bool strong: false
    color: strong ? Th.glassStrong : Th.glass
    radius: 14
    border.color: Th.edge
    border.width: 1
    Rectangle {
        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 1 }
        height: Math.min(parent.height / 2, 24)
        radius: parent.radius
        gradient: Gradient {
            GradientStop { position: 0; color: Qt.rgba(1, 1, 1, Th.dark ? 0.07 : 0.35) }
            GradientStop { position: 1; color: "transparent" }
        }
    }
}
