// Deslizador; «committed» al soltar (así no se guarda en cada paso)
import QtQuick
import QtQuick.Controls
import "."

Slider {
    id: s
    signal committed(real v)
    implicitWidth: 220
    implicitHeight: 26
    onPressedChanged: if (!pressed) committed(value)
    background: Rectangle {
        x: s.leftPadding; y: s.topPadding + s.availableHeight / 2 - height / 2
        width: s.availableWidth; height: 5; radius: 2.5
        color: Ui.track
        Rectangle { width: s.visualPosition * parent.width; height: parent.height; radius: 2.5; color: Ui.accent }
    }
    handle: Rectangle {
        x: s.leftPadding + s.visualPosition * (s.availableWidth - width)
        y: s.topPadding + s.availableHeight / 2 - height / 2
        width: 20; height: 20; radius: 10
        color: "#ffffff"
        border.color: "#33000000"
    }
}
