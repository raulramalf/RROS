// Menú desplegable de Lily: entries = [[texto, acción] | ["-"]]
import QtQuick
import QtQuick.Layouts
import "."

Glass {
    id: m
    property var entries: []
    signal picked(string action)
    strong: true
    radius: 12
    width: 240
    height: col.implicitHeight + 12
    ColumnLayout {
        id: col
        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 6 }
        spacing: 0
        Repeater {
            model: m.entries
            Item {
                Layout.fillWidth: true
                implicitHeight: modelData[0] === "-" ? 9 : 28
                Rectangle { visible: modelData[0] === "-"; anchors.centerIn: parent; width: parent.width - 16; height: 1; color: Th.edge }
                Rectangle {
                    visible: modelData[0] !== "-"
                    anchors.fill: parent
                    radius: 6
                    color: mi.containsMouse ? Th.accent : "transparent"
                    Txt { x: 10; anchors.verticalCenter: parent.verticalCenter; text: modelData[0]; color: mi.containsMouse ? "#ffffff" : Th.text }
                    MouseArea { id: mi; anchors.fill: parent; hoverEnabled: true; onClicked: { Ux.close(); m.picked(modelData[1]) } }
                }
            }
        }
    }
}
