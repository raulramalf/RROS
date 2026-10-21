// Lily Capturas: captura de la pantalla entera, de la ventana activa o de una zona (con Spectacle por debajo, sin su
// ventana), con retardo opcional. Las capturas se guardan en ~/Imágenes/Capturas y se ven aquí mismo.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

ApplicationWindow {
    id: win
    width: 900; height: 600
    visible: true
    title: "Capturas"
    color: Ui.window
    font.family: Ui.font
    readonly property string dir: Sys.env("HOME") + "/Imágenes/Capturas"
    property var shots: []
    property int delay: 0
    function load() { shots = Sys.listDir(dir, ["*.png", "*.jpg"]) }
    function take(mode) {
        var f = dir + "/Captura " + new Date().toLocaleString(Qt.locale("es_ES"), "yyyy-MM-dd HH.mm.ss") + ".png"
        Sys.writeFile(dir + "/.lily", "")
        win.visible = false
        Sys.run(["spectacle", "-b", "-n", mode, "-d", String(delay * 1000 + 300), "-o", f], function (r) {
            win.visible = true; load()
            if (r.code !== 0) Ui.toast("No se pudo capturar", true)
        })
    }
    Component.onCompleted: { Ui.reload(); load(); if (shotPath !== "") shotT.start() }
    Timer { id: shotT; interval: 1500; onTriggered: { Sys.saveShot(win, shotPath); Qt.quit() } }

    ColumnLayout {
        anchors { fill: parent; margins: 20 }
        spacing: 16
        Txt { text: "Capturas"; font.pixelSize: 22; font.weight: Font.Bold }
        RowLayout {
            spacing: 10
            Btn { text: "Pantalla entera"; kind: "primary"; onClicked: win.take("-f") }
            Btn { text: "Ventana activa"; onClicked: win.take("-a") }
            Btn { text: "Una zona"; onClicked: win.take("-r") }
            Item { Layout.fillWidth: true }
            Txt { text: "Esperar"; color: Ui.muted }
            Seg { options: [["0", "No"], ["3", "3 s"], ["10", "10 s"]]; value: String(win.delay); onPicked: function (v) { win.delay = parseInt(v) } }
        }
        Txt { text: win.shots.length ? win.shots.length + " capturas en Imágenes/Capturas" : "Todavía no hay capturas. También con la tecla Impr Pant."; color: Ui.muted }
        GridView {
            Layout.fillWidth: true; Layout.fillHeight: true
            clip: true
            cellWidth: Math.floor(width / 3); cellHeight: cellWidth * 0.66
            model: win.shots
            delegate: Item {
                width: GridView.view.cellWidth; height: GridView.view.cellHeight
                Rectangle {
                    anchors { fill: parent; margins: 6 }
                    radius: 10; color: Ui.card; border.color: sm.containsMouse ? Ui.accent : Ui.line; clip: true
                    Image { anchors { fill: parent; margins: 4 } source: "file://" + modelData.path; fillMode: Image.PreserveAspectFit; asynchronous: true; sourceSize.width: 400 }
                    MouseArea { id: sm; anchors.fill: parent; hoverEnabled: true; onClicked: Sys.detached(["xdg-open", modelData.path]) }
                }
            }
        }
    }
}
