// Lily Notas: notas rápidas con el diseño de Lily. Cada nota es un archivo de texto en ~/Documentos/Notas (sin bases
// de datos: se ven y se copian como cualquier archivo). Se guarda sola mientras escribes.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

ApplicationWindow {
    id: win
    width: 920; height: 620
    visible: true
    title: "Notas"
    color: Ui.window
    font.family: Ui.font
    readonly property string dir: Sys.env("HOME") + "/Documentos/Notas"
    property var notes: []
    property string current: ""
    property string filter: ""
    function load() {
        notes = Sys.listDir(dir, ["*.md", "*.txt"])
        if (current === "" && notes.length) open(notes[0].path)
    }
    function open(path) { save(); current = path; editor.text = Sys.readFile(path) }
    function save() { if (current !== "" && editor.dirty) { Sys.writeFile(current, editor.text); editor.dirty = false } }
    function create() {
        save()
        var p = dir + "/Nota " + new Date().toLocaleString(Qt.locale("es_ES"), "yyyy-MM-dd HH.mm.ss") + ".md"
        Sys.writeFile(p, ""); current = p; editor.text = ""; load(); editor.forceActiveFocus()
    }
    function remove() { if (current === "") return; Sys.removeFile(current); current = ""; editor.text = ""; load() }
    Component.onCompleted: { Ui.reload(); load(); if (shotPath !== "") shotT.start() }
    Timer { id: shotT; interval: 1500; onTriggered: { Sys.saveShot(win, shotPath); Qt.quit() } }
    onClosing: save()
    Timer { interval: 1200; running: editor.dirty; onTriggered: { win.save(); win.load() } }
    Shortcut { sequence: StandardKey.New; onActivated: win.create() }
    Shortcut { sequence: StandardKey.Save; onActivated: win.save() }

    RowLayout {
        anchors.fill: parent
        spacing: 0
        // ---- lista ----
        Rectangle {
            Layout.preferredWidth: 280; Layout.fillHeight: true
            color: Ui.sidebar
            ColumnLayout {
                anchors { fill: parent; margins: 12 }
                spacing: 10
                RowLayout {
                    Txt { text: "Notas"; font.pixelSize: 20; font.weight: Font.Bold; Layout.fillWidth: true }
                    Btn { text: "Nueva"; kind: "primary"; onClicked: win.create() }
                }
                Field { Layout.fillWidth: true; placeholderText: "Buscar"; onTextChanged: win.filter = text.toLowerCase() }
                ListView {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    clip: true; spacing: 4
                    model: win.notes.filter(function (n) { return win.filter === "" || n.name.toLowerCase().indexOf(win.filter) >= 0 || Sys.readFile(n.path).toLowerCase().indexOf(win.filter) >= 0 })
                    delegate: Rectangle {
                        width: ListView.view.width; height: 58; radius: 10
                        color: win.current === modelData.path ? Ui.accent : nm.containsMouse ? Ui.cardHover : "transparent"
                        property string first: Sys.readFile(modelData.path).split("\n")[0]
                        Column {
                            anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; margins: 12 }
                            Txt { width: parent.width; elide: Text.ElideRight; font.weight: Font.DemiBold
                                  text: parent.parent.first.replace(/^#+\s*/, "") || "Nota vacía"; color: win.current === modelData.path ? Ui.accentText : Ui.textColor }
                            Txt { width: parent.width; elide: Text.ElideRight; font.pixelSize: Ui.fs - 2
                                  text: Qt.formatDateTime(modelData.modified, "d MMM yyyy, HH:mm"); color: win.current === modelData.path ? Ui.accentText : Ui.muted }
                        }
                        MouseArea { id: nm; anchors.fill: parent; hoverEnabled: true; onClicked: win.open(modelData.path) }
                    }
                }
            }
        }
        // ---- editor ----
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.margins: 20
            RowLayout {
                visible: win.current !== ""
                Txt { text: editor.dirty ? "Guardando…" : "Guardada"; color: Ui.muted; font.pixelSize: Ui.fs - 1; Layout.fillWidth: true }
                Btn { text: "Borrar"; kind: "danger"; onClicked: win.remove() }
            }
            ScrollView {
                Layout.fillWidth: true; Layout.fillHeight: true
                TextArea {
                    id: editor
                    property bool dirty: false
                    enabled: win.current !== ""
                    placeholderText: win.current === "" ? "Crea una nota con «Nueva» (Ctrl+N)" : "Escribe… La primera línea es el título."
                    color: Ui.textColor; placeholderTextColor: Ui.muted
                    font.family: Ui.font; font.pixelSize: 15
                    wrapMode: TextArea.Wrap
                    selectionColor: Ui.accent
                    background: null
                    onTextChanged: if (activeFocus) dirty = true
                }
            }
        }
    }
}
