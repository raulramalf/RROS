// Botón: kind = "normal", "primary" o "danger"
import QtQuick
import "."

Rectangle {
    id: b
    property string text
    property string kind: "normal"
    property bool busy: false
    property bool enabled2: true
    signal clicked()
    implicitWidth: Math.max(80, t.implicitWidth + 28)
    implicitHeight: 30
    radius: 8
    opacity: enabled2 ? 1 : 0.45
    color: kind === "primary" ? (ma.pressed ? Qt.darker(Ui.accent, 1.15) : Ui.accent)
         : kind === "danger" ? (ma.containsMouse ? Qt.tint(Ui.danger, "#20000000") : Qt.rgba(Qt.color(Ui.danger).r, Qt.color(Ui.danger).g, Qt.color(Ui.danger).b, 0.16))
         : (ma.pressed ? Ui.track : ma.containsMouse ? Qt.tint(Ui.field, Ui.dark ? "#14ffffff" : "#0c000000") : Ui.field)
    border.color: kind === "normal" ? Ui.line : "transparent"
    Txt {
        id: t
        anchors.centerIn: parent
        text: b.busy ? "…" : b.text
        color: b.kind === "primary" ? Ui.accentText : b.kind === "danger" ? Ui.danger : Ui.textColor
        font.weight: Font.Medium
    }
    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
        enabled: b.enabled2 && !b.busy
        cursorShape: Qt.PointingHandCursor
        onClicked: b.clicked()
    }
    Accessible.role: Accessible.Button
    Accessible.name: text
}
