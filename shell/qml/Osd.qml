// Aviso de volumen, brillo y teclado
import QtQuick
import "."

Glass {
    id: osd
    strong: true
    width: 280; height: 56; radius: 28
    opacity: 0
    visible: opacity > 0
    property string icon
    property int percent: -1
    property string text
    Behavior on opacity { NumberAnimation { duration: 150 } }
    Connections {
        target: Sh
        function onOsd(icon, percent, text) { osd.icon = icon; osd.percent = percent; osd.text = text; osd.opacity = 1; hide.restart() }
    }
    Timer { id: hide; interval: 1600; onTriggered: osd.opacity = 0 }
    Glyph {
        id: g
        x: 18; anchors.verticalCenter: parent.verticalCenter
        width: 20; height: 20
        name: osd.icon.indexOf("brightness") >= 0 ? "sun" : osd.icon.indexOf("volume") >= 0 || osd.icon.indexOf("audio") >= 0 ? "volume" : "control"
        level: osd.percent / 100
        flag: osd.icon.indexOf("muted") >= 0
    }
    Rectangle {
        visible: osd.percent >= 0
        anchors { left: g.right; leftMargin: 14; right: parent.right; rightMargin: 22; verticalCenter: parent.verticalCenter }
        height: 6; radius: 3; color: Th.tile
        Rectangle { width: parent.width * Math.min(1, osd.percent / 100); height: parent.height; radius: 3; color: Th.text }
    }
    Txt { visible: osd.percent < 0; anchors { left: g.right; leftMargin: 14; verticalCenter: parent.verticalCenter } text: osd.text }
}
