// Pantalla de carga de RROS (mientras Plasma prepara el escritorio): el logotipo y una barra que avanza.
import QtQuick 2.15

Rectangle {
    id: root
    color: "#0b1020"
    property int stage

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#1b1446" }
            GradientStop { position: 0.6; color: "#0b1020" }
            GradientStop { position: 1.0; color: "#06202a" }
        }
    }
    Image {
        id: logo
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -30
        source: "images/rros.svg"
        sourceSize.width: 112
        sourceSize.height: 112
        opacity: 0
        Component.onCompleted: opacity = 1
        Behavior on opacity { NumberAnimation { duration: 400 } }
    }
    Text {
        anchors { top: logo.bottom; topMargin: 22; horizontalCenter: parent.horizontalCenter }
        text: "RROS"
        color: "#e6ecff"
        font.family: "Inter"
        font.pixelSize: 26
        font.weight: Font.Black
        font.letterSpacing: 4
    }
    Rectangle {
        id: track
        anchors { top: logo.bottom; topMargin: 76; horizontalCenter: parent.horizontalCenter }
        width: 180; height: 4; radius: 2
        color: "#26305a"
        Rectangle {
            width: parent.width * Math.min(1, root.stage / 6)
            height: parent.height; radius: 2
            color: "#7c5cff"
            Behavior on width { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }
        }
    }
}
