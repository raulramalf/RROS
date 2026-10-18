// Control segmentado: options = [[valor, etiqueta], ...]
import QtQuick
import QtQuick.Layouts
import "."

Rectangle {
    id: seg
    property var options: []
    property string value
    signal picked(string v)
    implicitWidth: lay.implicitWidth + 6
    implicitHeight: 30
    radius: 8
    color: Ui.field
    RowLayout {
        id: lay
        anchors.centerIn: parent
        spacing: 2
        Repeater {
            model: seg.options
            Rectangle {
                property bool on: seg.value === modelData[0]
                implicitWidth: Math.max(52, lbl.implicitWidth + 22)
                implicitHeight: 24
                radius: 6
                color: on ? (Ui.dark ? Qt.tint(Ui.card, "#26ffffff") : "#ffffff") : (ma.containsMouse ? Ui.cardHover : "transparent")
                border.color: on ? Ui.line : "transparent"
                Txt { id: lbl; anchors.centerIn: parent; text: modelData[1]; font.pixelSize: Ui.fs - 1; font.weight: on ? Font.DemiBold : Font.Normal }
                MouseArea {
                    id: ma
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: { seg.value = modelData[0]; seg.picked(modelData[0]) }
                }
            }
        }
    }
}
