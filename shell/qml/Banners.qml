// Avisos emergentes (arriba a la derecha)
import QtQuick
import QtQuick.Layouts
import "."

Column {
    id: bn
    width: 360
    spacing: 8
    property var shown: []
    Connections {
        target: Notifs
        function onPopup(id, app, icon, summary, body, timeout) {
            var l = bn.shown.filter(function (n) { return n.id !== id })
            l.unshift({ id: id, app: app, icon: icon, summary: summary, body: body, timeout: timeout })
            bn.shown = l.slice(0, 4)
        }
    }
    function drop(id) { shown = shown.filter(function (n) { return n.id !== id }) }
    Repeater {
        model: bn.shown
        Glass {
            strong: true
            width: bn.width
            height: c.implicitHeight + 22
            radius: 16
            RowLayout {
                id: c
                anchors { left: parent.left; right: parent.right; top: parent.top; margins: 11 }
                spacing: 10
                Ico { size: 36; name: modelData.icon || "dialog-information"; Layout.alignment: Qt.AlignTop }
                ColumnLayout {
                    spacing: 2
                    Layout.fillWidth: true
                    Txt { text: modelData.app; color: Th.muted; font.pixelSize: 11; Layout.fillWidth: true }
                    Txt { text: modelData.summary; font.weight: Font.DemiBold; Layout.fillWidth: true }
                    Txt { text: modelData.body; color: Th.muted; textFormat: Text.PlainText; wrapMode: Text.WordWrap; maximumLineCount: 4; Layout.fillWidth: true; visible: text !== "" }
                }
            }
            MouseArea { anchors.fill: parent; onClicked: { Notifs.invoke(modelData.id, "default"); bn.drop(modelData.id) } }
            Timer { interval: modelData.timeout > 0 ? modelData.timeout : 8000; running: modelData.timeout !== 0; onTriggered: bn.drop(modelData.id) }
        }
    }
}
