// Interruptor de Lily
import QtQuick
import "."

Rectangle {
    id: sw
    property bool checked: false
    signal toggled(bool on)
    width: 42; height: 25; radius: height / 2
    color: checked ? Ui.accent : Ui.track
    Behavior on color { ColorAnimation { duration: 120 } }
    Rectangle {
        width: 21; height: 21; radius: 10.5
        y: 2
        x: sw.checked ? sw.width - width - 2 : 2
        color: "#ffffff"
        Behavior on x { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
        Rectangle { anchors.fill: parent; radius: parent.radius; color: "transparent"; border.color: "#22000000" }
    }
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: { sw.checked = !sw.checked; sw.toggled(sw.checked) }
    }
    Accessible.role: Accessible.CheckBox
    Accessible.checked: checked
}
