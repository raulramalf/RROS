// Tarjeta de ajustes: filas separadas por una línea fina, como en las preferencias de macOS
import QtQuick
import QtQuick.Layouts
import "."

Rectangle {
    id: card
    default property alias content: col.data
    property alias spacing: col.spacing
    Layout.fillWidth: true
    implicitHeight: col.implicitHeight + 2
    radius: Ui.radius
    color: Ui.card
    border.color: Ui.line
    border.width: 1
    ColumnLayout {
        id: col
        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 1 }
        spacing: 0
    }
}
