// Diálogo con campos: fields = [{id, label, password, value, placeholder}]
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

Popup {
    id: d
    property string title
    property string text
    property var fields: []
    property string okText: "Aceptar"
    property var callback: null
    property var values: ({})
    modal: true
    anchors.centerIn: Overlay.overlay
    width: 420
    padding: 22
    background: Rectangle { radius: 14; color: Ui.card; border.color: Ui.line }
    Overlay.modal: Rectangle { color: "#66000000" }

    function ask(t, msg, flds, ok, cb) {
        title = t; text = msg; fields = flds; okText = ok || "Aceptar"; callback = cb
        var v = {}
        for (var i = 0; i < flds.length; i++) v[flds[i].id] = flds[i].value || ""
        values = v
        open()
    }
    function accept() {
        var cb = callback, v = values
        close()
        if (cb) cb(v)
    }

    contentItem: ColumnLayout {
        spacing: 12
        Txt { text: d.title; font.pixelSize: 17; font.weight: Font.Bold }
        Txt { text: d.text; visible: text !== ""; color: Ui.muted; wrapMode: Text.WordWrap; elide: Text.ElideNone; Layout.fillWidth: true }
        Repeater {
            model: d.fields
            ColumnLayout {
                spacing: 4
                Layout.fillWidth: true
                Txt { text: modelData.label; font.pixelSize: Ui.fs - 1; color: Ui.muted }
                Field {
                    Layout.fillWidth: true
                    text: modelData.value || ""
                    placeholderText: modelData.placeholder || ""
                    echoMode: modelData.password ? TextInput.Password : TextInput.Normal
                    focus: index === 0
                    onTextChanged: d.values[modelData.id] = text
                    onAccepted: d.accept()
                }
            }
        }
        RowLayout {
            Layout.topMargin: 6
            Item { Layout.fillWidth: true }
            Btn { text: "Cancelar"; onClicked: d.close() }
            Btn { text: d.okText; kind: "primary"; onClicked: d.accept() }
        }
    }
}
