// Lily Archivos: gestor de archivos sencillo con lugares a la izquierda, ruta clicable, cuadrícula o lista, búsqueda,
// carpeta nueva, renombrar y mover a la papelera (gio trash). Abre los archivos con su aplicación (xdg-open) y las
// imágenes con Lily Fotos.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

ApplicationWindow {
    id: win
    width: 1000; height: 640
    visible: true
    title: "Archivos"
    color: Ui.window
    font.family: Ui.font
    readonly property string home: Sys.env("HOME")
    readonly property var imgs: ["png", "jpg", "jpeg", "webp", "gif", "bmp", "svg"]
    property string cwd: appArg !== "" && Sys.exists(appArg) ? appArg : home
    property var items: []
    property var back: []
    property string filter: ""
    property bool hidden: false
    property bool grid: true
    property string sel: ""

    function load() { items = Sys.entries(cwd, hidden); sel = "" }
    function go(p) { if (p === cwd) return; back.push(cwd); cwd = p; filter = ""; search.text = ""; load() }
    function up() { var i = cwd.lastIndexOf("/"); if (i > 0) go(cwd.substring(0, i)); else if (cwd !== "/") go("/") }
    function activate(e) {
        if (e.dir) go(e.path)
        else if (imgs.indexOf(e.suffix) >= 0) Sys.detached(["rros-ajustes", "--app", "fotos", e.path])
        else Sys.detached(["xdg-open", e.path])
    }
    function icon(e) {
        if (e.dir) return "📁"
        if (imgs.indexOf(e.suffix) >= 0) return "🖼"
        if (["mp4", "mkv", "webm", "avi", "mov"].indexOf(e.suffix) >= 0) return "🎞"
        if (["mp3", "ogg", "flac", "wav", "m4a", "opus"].indexOf(e.suffix) >= 0) return "🎵"
        if (["zip", "tar", "gz", "xz", "zst", "7z", "rar"].indexOf(e.suffix) >= 0) return "🗜"
        if (e.suffix === "pdf") return "📕"
        return "📄"
    }
    function human(n) {
        var u = ["B", "KB", "MB", "GB", "TB"], i = 0
        while (n >= 1024 && i < u.length - 1) { n /= 1024; i++ }
        return (i ? n.toFixed(1) : n) + " " + u[i]
    }
    function trash() {
        if (sel === "") return
        Sys.run(["gio", "trash", sel], function () { load(); Ui.toast("Movido a la papelera", false) })
    }
    function newFolder() {
        var n = "Carpeta nueva", i = 2
        while (Sys.exists(cwd + "/" + n)) n = "Carpeta nueva " + i++
        Sys.makeDir(cwd + "/" + n); load(); sel = cwd + "/" + n; renameField.start()
    }

    Component.onCompleted: { Ui.reload(); load(); if (shotPath !== "") shotT.start() }
    Timer { id: shotT; interval: 1500; onTriggered: { Sys.saveShot(win, shotPath); Qt.quit() } }
    Shortcut { sequence: "Alt+Up"; onActivated: win.up() }
    Shortcut { sequence: "Alt+Left"; onActivated: if (win.back.length) { win.cwd = win.back.pop(); win.load() } }
    Shortcut { sequence: "Ctrl+H"; onActivated: { win.hidden = !win.hidden; win.load() } }
    Shortcut { sequence: "Ctrl+Shift+N"; onActivated: win.newFolder() }
    Shortcut { sequence: "F2"; onActivated: if (win.sel !== "") renameField.start() }
    Shortcut { sequences: [StandardKey.Delete]; onActivated: win.trash() }
    Shortcut { sequence: StandardKey.Find; onActivated: search.forceActiveFocus() }
    Shortcut { sequence: "F5"; onActivated: win.load() }
    Connections { target: Ui; function onToast(text, error) { toastT.text = text; toastT.opacity = 1; toastHide.restart() } }

    RowLayout {
        anchors.fill: parent
        spacing: 0
        // ---- lugares ----
        Rectangle {
            Layout.preferredWidth: 210; Layout.fillHeight: true
            color: Ui.sidebar
            Rectangle { anchors { right: parent.right; top: parent.top; bottom: parent.bottom } width: 1; color: Ui.line }
            ColumnLayout {
                anchors { fill: parent; margins: 10; topMargin: 16 }
                spacing: 2
                Txt { text: "Archivos"; font.pixelSize: 20; font.weight: Font.Bold; Layout.leftMargin: 8; Layout.bottomMargin: 10 }
                Repeater {
                    model: [["🏠", "Carpeta personal", ""], ["🖥", "Escritorio", "/Escritorio"], ["📄", "Documentos", "/Documentos"],
                            ["⬇", "Descargas", "/Descargas"], ["🖼", "Imágenes", "/Imágenes"], ["🎵", "Música", "/Música"],
                            ["🎞", "Vídeos", "/Vídeos"], ["🗑", "Papelera", "/.local/share/Trash/files"], ["💽", "Todo el equipo", "/"]]
                    Rectangle {
                        readonly property string p: modelData[2] === "/" ? "/" : win.home + modelData[2]
                        visible: p === "/" || Sys.exists(p)
                        Layout.fillWidth: true; Layout.preferredHeight: visible ? 32 : 0
                        radius: 8
                        color: win.cwd === p ? Ui.accent : pm.containsMouse ? Ui.cardHover : "transparent"
                        RowLayout {
                            anchors { fill: parent; leftMargin: 10 }
                            spacing: 10
                            Txt { text: modelData[0] }
                            Txt { text: modelData[1]; color: win.cwd === parent.parent.p ? Ui.accentText : Ui.textColor; Layout.fillWidth: true }
                        }
                        MouseArea { id: pm; anchors.fill: parent; hoverEnabled: true; onClicked: win.go(parent.p) }
                    }
                }
                Item { Layout.fillHeight: true }
            }
        }
        // ---- carpeta ----
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            Layout.margins: 14
            spacing: 10
            RowLayout {
                spacing: 6
                Btn { text: "←"; enabled: win.back.length > 0; onClicked: { win.cwd = win.back.pop(); win.load() } }
                Btn { text: "↑"; enabled: win.cwd !== "/"; onClicked: win.up() }
                // ruta: cada trozo lleva a esa carpeta
                Flickable {
                    Layout.fillWidth: true; Layout.preferredHeight: 32
                    contentWidth: crumbs.width; clip: true
                    Row {
                        id: crumbs
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 2
                        Repeater {
                            model: {
                                var parts = win.cwd.split("/").filter(function (x) { return x !== "" }), out = [["/", "/"]], acc = ""
                                for (var i = 0; i < parts.length; i++) { acc += "/" + parts[i]; out.push([parts[i], acc]) }
                                return out
                            }
                            Rectangle {
                                width: ct.implicitWidth + 16; height: 28; radius: 7
                                color: cm.containsMouse ? Ui.cardHover : "transparent"
                                Txt { id: ct; anchors.centerIn: parent; text: modelData[0] + (index ? "  ›" : ""); font.weight: index === 0 ? Font.Normal : Font.DemiBold }
                                MouseArea { id: cm; anchors.fill: parent; hoverEnabled: true; onClicked: win.go(modelData[1]) }
                            }
                        }
                    }
                }
                Field { id: search; Layout.preferredWidth: 200; placeholderText: "Buscar aquí"; onTextChanged: win.filter = text.toLowerCase() }
                Btn { text: win.grid ? "☰" : "▦"; onClicked: win.grid = !win.grid }
                Btn { text: "Carpeta nueva"; onClicked: win.newFolder() }
                Btn { text: "Papelera"; kind: "danger"; enabled: win.sel !== ""; onClicked: win.trash() }
            }
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true
                radius: 12; color: Ui.card; border.color: Ui.line
                GridView {
                    id: view
                    anchors { fill: parent; margins: 10 }
                    clip: true
                    readonly property var shown: win.items.filter(function (e) { return win.filter === "" || e.name.toLowerCase().indexOf(win.filter) >= 0 })
                    model: shown
                    cellWidth: win.grid ? 118 : width
                    cellHeight: win.grid ? 112 : 36
                    ScrollBar.vertical: ScrollBar {}
                    delegate: Rectangle {
                        width: view.cellWidth - 6; height: view.cellHeight - 4
                        radius: 9
                        color: win.sel === modelData.path ? Qt.rgba(Qt.color(Ui.accent).r, Qt.color(Ui.accent).g, Qt.color(Ui.accent).b, 0.3)
                                                          : em.containsMouse ? Ui.cardHover : "transparent"
                        readonly property bool isImg: win.imgs.indexOf(modelData.suffix) >= 0 && modelData.size < 20000000
                        // cuadrícula: miniatura o emoji arriba y nombre debajo
                        Column {
                            visible: win.grid
                            anchors.centerIn: parent
                            spacing: 6
                            Item {
                                width: 64; height: 56; anchors.horizontalCenter: parent.horizontalCenter
                                Image { anchors.fill: parent; visible: parent.parent.parent.isImg && win.grid; asynchronous: true
                                        source: visible ? "file://" + modelData.path : ""; sourceSize: Qt.size(128, 112); fillMode: Image.PreserveAspectFit }
                                Txt { anchors.centerIn: parent; visible: !parent.parent.parent.isImg; text: win.icon(modelData); font.pixelSize: 40 }
                            }
                            Txt { width: 104; horizontalAlignment: Text.AlignHCenter; text: modelData.name; elide: Text.ElideMiddle; font.pixelSize: Ui.fs - 1 }
                        }
                        // lista: icono, nombre, tamaño y fecha
                        RowLayout {
                            visible: !win.grid
                            anchors { fill: parent; leftMargin: 10; rightMargin: 10 }
                            spacing: 10
                            Txt { text: win.icon(modelData) }
                            Txt { text: modelData.name; elide: Text.ElideMiddle; Layout.fillWidth: true }
                            Txt { text: modelData.dir ? "" : win.human(modelData.size); color: Ui.muted; Layout.preferredWidth: 80; horizontalAlignment: Text.AlignRight }
                            Txt { text: Qt.formatDateTime(modelData.modified, "d MMM yyyy HH:mm"); color: Ui.muted; Layout.preferredWidth: 140 }
                        }
                        MouseArea {
                            id: em; anchors.fill: parent; hoverEnabled: true
                            onClicked: win.sel = modelData.path
                            onDoubleClicked: win.activate(modelData)
                        }
                    }
                    Txt { anchors.centerIn: parent; visible: view.count === 0; color: Ui.muted
                          text: win.filter !== "" ? "Nada coincide con «" + win.filter + "»" : "Carpeta vacía" }
                }
            }
            RowLayout {
                Txt { text: view.count + (view.count === 1 ? " elemento" : " elementos") + (win.hidden ? " · con ocultos" : ""); color: Ui.muted; font.pixelSize: Ui.fs - 1; Layout.fillWidth: true }
                // renombrar el elemento elegido (F2)
                Field {
                    id: renameField
                    visible: false
                    Layout.preferredWidth: 280
                    function start() { text = win.sel.substring(win.sel.lastIndexOf("/") + 1); visible = true; forceActiveFocus(); selectAll() }
                    onAccepted: {
                        var to = win.cwd + "/" + text
                        if (text !== "" && text.indexOf("/") < 0 && Sys.rename(win.sel, to)) { win.load(); win.sel = to }
                        else Ui.toast("No se pudo renombrar", true)
                        visible = false
                    }
                    Keys.onEscapePressed: visible = false
                }
                Btn { text: "Renombrar"; enabled: win.sel !== ""; onClicked: renameField.start() }
                Btn { text: "Terminal aquí"; onClicked: Sys.detached(["konsole", "--workdir", win.cwd]) }
            }
        }
    }
    Rectangle {
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 60 }
        width: toastT.implicitWidth + 32; height: 38; radius: 10; color: Ui.card; border.color: Ui.line
        opacity: toastT.opacity; visible: opacity > 0
        Txt { id: toastT; anchors.centerIn: parent; opacity: 0; Behavior on opacity { NumberAnimation { duration: 180 } } }
        Timer { id: toastHide; interval: 2500; onTriggered: toastT.opacity = 0 }
    }
}
