// Aviso dentro de una página
import QtQuick
import QtQuick.Layouts
import "."

Rectangle {
    id: n
    property string text
    property string kind: "info"     // info, warn, error, ok
    property color tone: kind === "warn" ? Ui.warn : kind === "error" ? Ui.danger : kind === "ok" ? Ui.ok : Ui.accent
    Layout.fillWidth: true
    implicitHeight: t.implicitHeight + 22
    radius: 10
    color: Qt.rgba(Qt.color(tone).r, Qt.color(tone).g, Qt.color(tone).b, 0.13)
    border.color: Qt.rgba(Qt.color(tone).r, Qt.color(tone).g, Qt.color(tone).b, 0.35)
    Txt {
        id: t
        anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; margins: 12 }
        text: n.text
        wrapMode: Text.WordWrap
        elide: Text.ElideNone
    }
}
