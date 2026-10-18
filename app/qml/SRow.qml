// Fila de una tarjeta: título, explicación y el control a la derecha
import QtQuick
import QtQuick.Layouts
import "."

Item {
    id: row
    property string title
    property string subtitle
    property string icon            // icono de sección (qrc) opcional
    property bool separator: true
    property bool enabled2: true
    default property alias control: slot.data
    signal clicked()
    property bool clickable: false
    Layout.fillWidth: true
    implicitHeight: Math.max(Ui.rowH, texts.implicitHeight + 22, slot.implicitHeight + 20)
    opacity: enabled2 ? 1 : 0.45

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: Ui.radius - 2
        color: mouse.containsMouse && row.clickable ? Ui.cardHover : "transparent"
    }
    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: row.clickable
        enabled: row.clickable
        cursorShape: row.clickable ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: row.clicked()
    }
    RowLayout {
        anchors { fill: parent; leftMargin: 16; rightMargin: 14 }
        spacing: 12
        Image {
            visible: row.icon !== ""
            source: row.icon
            sourceSize: Qt.size(28, 28)
            Layout.preferredWidth: 28
            Layout.preferredHeight: 28
        }
        ColumnLayout {
            id: texts
            spacing: 2
            Layout.fillWidth: true
            Txt { text: row.title; Layout.fillWidth: true; visible: text !== "" }
            Txt {
                text: row.subtitle
                visible: text !== ""
                color: Ui.muted
                font.pixelSize: Ui.fs - 1
                wrapMode: Text.WordWrap
                elide: Text.ElideNone
                Layout.fillWidth: true
            }
        }
        Item {
            id: slot
            implicitWidth: childrenRect.width
            implicitHeight: childrenRect.height
            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
        }
    }
    Rectangle {
        visible: row.separator
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom; leftMargin: 16 }
        height: 1
        color: Ui.line
    }
}
