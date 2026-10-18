// Página con desplazamiento: título, subtítulo y el contenido centrado
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

Flickable {
    id: page
    property string title
    property string subtitle
    property string icon
    default property alias content: body.data
    signal shown()
    contentWidth: width
    contentHeight: wrap.implicitHeight + 56
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

    ColumnLayout {
        id: wrap
        width: Math.min(page.width - 64, 720)
        x: (page.width - width) / 2
        y: 28
        spacing: 18
        RowLayout {
            spacing: 14
            Image { visible: page.icon !== ""; source: page.icon; sourceSize: Qt.size(44, 44); Layout.preferredWidth: 44; Layout.preferredHeight: 44 }
            ColumnLayout {
                spacing: 2
                Txt { text: page.title; font.pixelSize: 24; font.weight: Font.Bold }
                Txt { text: page.subtitle; visible: text !== ""; color: Ui.muted; wrapMode: Text.WordWrap; elide: Text.ElideNone; Layout.fillWidth: true }
                Layout.fillWidth: true
            }
            Layout.fillWidth: true
        }
        ColumnLayout {
            id: body
            spacing: 18
            Layout.fillWidth: true
        }
    }
}
