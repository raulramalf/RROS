// Lista desplegable: model = [[valor, etiqueta], ...]
import QtQuick
import QtQuick.Controls
import "."

ComboBox {
    id: c
    property var options: []
    property string value
    signal picked(string v)
    implicitWidth: 220
    implicitHeight: 30
    model: options
    textRole: ""
    displayText: { for (var i = 0; i < options.length; i++) if (options[i][0] === value) return options[i][1]; return value }
    font.family: Ui.font
    font.pixelSize: Ui.fs
    onActivated: function (i) { c.value = options[i][0]; c.picked(options[i][0]) }
    contentItem: Txt { leftPadding: 10; rightPadding: 26; text: c.displayText; verticalAlignment: Text.AlignVCenter }
    indicator: Txt { x: c.width - width - 10; anchors.verticalCenter: parent.verticalCenter; text: "⌄"; color: Ui.muted; font.pixelSize: 15 }
    background: Rectangle { radius: 8; color: Ui.field; border.color: c.visualFocus ? Ui.accent : Ui.line }
    delegate: ItemDelegate {
        width: c.width
        height: 30
        highlighted: c.highlightedIndex === index
        contentItem: Txt { text: modelData[1]; font.weight: modelData[0] === c.value ? Font.DemiBold : Font.Normal }
        background: Rectangle { radius: 6; color: highlighted ? Ui.accent : "transparent"; opacity: highlighted ? 0.35 : 1 }
    }
    popup: Popup {
        y: c.height + 4
        width: c.width
        implicitHeight: Math.min(contentItem.implicitHeight + 8, 320)
        padding: 4
        contentItem: ListView { clip: true; implicitHeight: contentHeight; model: c.popup.visible ? c.delegateModel : null; currentIndex: c.highlightedIndex }
        background: Rectangle { radius: 10; color: Ui.card; border.color: Ui.line }
    }
}
