// Pantalla de carga de Lily (mientras Plasma prepara el escritorio): el logotipo y una barra que avanza.
import QtQuick 2.15

Rectangle {
    id: root
    color: "#140609"
    property int stage

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#2a0a12" }
            GradientStop { position: 0.6; color: "#140609" }
            GradientStop { position: 1.0; color: "#0b0306" }
        }
    }
    Image {
        id: logo
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -30
        source: "images/rros.png"
        width: 128; height: 128
        smooth: true; mipmap: true
        opacity: 0
        Component.onCompleted: opacity = 1
        Behavior on opacity { NumberAnimation { duration: 400 } }
    }
    Rectangle {
        id: track
        anchors { top: logo.bottom; topMargin: 40; horizontalCenter: parent.horizontalCenter }
        width: 180; height: 4; radius: 2
        color: "#3d1a22"
        Rectangle {
            width: parent.width * Math.min(1, root.stage / 6)
            height: parent.height; radius: 2
            color: "#e9d3b4"
            Behavior on width { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }
        }
    }
}
