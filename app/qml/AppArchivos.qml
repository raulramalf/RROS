// Lily Archivos: gestor de archivos sencillo con lugares a la izquierda, ruta clicable, cuadrícula o lista, búsqueda,
// carpeta nueva, renombrar y mover a la papelera (gio trash). Abre los archivos con su aplicación (xdg-open) y las
// imágenes con Lily Fotos. Selección múltiple (Ctrl, Mayús, Ctrl+A y recuadro arrastrando en un hueco) y arrastrar y
// soltar: a una carpeta (mover; con Ctrl, copiar), a otra aplicación o desde otra aplicación (se copia aquí).
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
    property string sel: ""          // el último elegido (renombrar)
    property var picked: ({})        // todos los elegidos: ruta -> true
    property int anchorIx: -1

    function load() { items = Sys.entries(cwd, hidden); sel = ""; picked = ({}) }
    function selected() { var out = []; for (var k in picked) out.push(k); return out }
    function only(p) { var s = {}; if (p !== "") s[p] = true; picked = s; sel = p }
    function toggleSel(p) { var s = Object.assign({}, picked); if (s[p]) delete s[p]; else s[p] = true; picked = s; sel = s[p] ? p : "" }
    function rangeSel(a, b) { var s = Object.assign({}, picked), l = view.shown; for (var i = Math.min(a, b); i <= Math.max(a, b); i++) s[l[i].path] = true; picked = s }
    function selectAll() { var s = {}; view.shown.forEach(function (e) { s[e.path] = true }); picked = s }
    function uri(p) { return "file://" + encodeURI(p).replace(/#/g, "%23").replace(/\?/g, "%3F") }
    function dropPaths(drop) {
        var out = []
        if (drop.hasUrls) for (var i = 0; i < drop.urls.length; i++) {
            var u = String(drop.urls[i])
            if (u.indexOf("file://") === 0) out.push(decodeURIComponent(u.substring(7)))
        }
        return out
    }
    // mover o copiar dentro de una carpeta, sin sobrescribir; nunca una carpeta dentro de sí misma
    function transfer(paths, dest, copy) {
        paths = paths.filter(function (p) { return p !== dest && p.substring(0, p.lastIndexOf("/")) !== dest && dest.indexOf(p + "/") !== 0 })
        if (!paths.length) return
        Sys.run(copy ? ["cp", "-r", "-n", "-t", dest, "--"].concat(paths) : ["mv", "-n", "-t", dest, "--"].concat(paths), function (r) {
            win.load(); Ui.toast(r.code === 0 ? (paths.length === 1 ? "1 elemento " : paths.length + " elementos ") + (copy ? "copiado" : "movido") + (paths.length === 1 ? "" : "s")
                                              : "No se pudo " + (copy ? "copiar" : "mover"), r.code !== 0)
        })
    }
    function go(p) { if (p === cwd) return; back.push(cwd); cwd = p; filter = ""; search.text = ""; load() }
    function up() { var i = cwd.lastIndexOf("/"); if (i > 0) go(cwd.substring(0, i)); else if (cwd !== "/") go("/") }
    function activate(e) {
        if (e.dir) go(e.path)
        else if (imgs.indexOf(e.suffix) >= 0) Sys.detached(["rros-ajustes", "--app", "fotos", e.path])
        else Sys.detached(["xdg-open", e.path])
    }
    // icono propio de Lily según el tipo (app/icons/fm-*.svg); las carpetas conocidas llevan su símbolo
    function icon(e) {
        var ext = e.suffix
        if (e.dir) {
            var sp = { "Escritorio": "escritorio", "Documentos": "documentos", "Descargas": "descargas", "Imágenes": "imagenes", "Música": "musica", "Vídeos": "videos" }
            return "fm-carpeta" + (e.path === win.home + "/" + e.name && sp[e.name] ? "-" + sp[e.name] : "")
        }
        if (imgs.indexOf(ext) >= 0) return "fm-imagen"
        if (["mp4", "mkv", "webm", "avi", "mov"].indexOf(ext) >= 0) return "fm-video"
        if (["mp3", "ogg", "flac", "wav", "m4a", "opus"].indexOf(ext) >= 0) return "fm-audio"
        if (["zip", "tar", "gz", "xz", "zst", "7z", "rar", "bz2"].indexOf(ext) >= 0) return "fm-comprimido"
        if (ext === "pdf") return "fm-pdf"
        if (["ods", "xlsx", "xls", "csv"].indexOf(ext) >= 0) return "fm-hoja"
        if (["sh", "py", "js", "c", "h", "cpp", "qml", "json", "toml", "yml", "yaml", "html", "css", "rs", "go"].indexOf(ext) >= 0) return "fm-codigo"
        if (["appimage", "run", "bin", "exe"].indexOf(ext) >= 0) return "fm-ejecutable"
        return "fm-texto"
    }
    function human(n) {
        var u = ["B", "KB", "MB", "GB", "TB"], i = 0
        while (n >= 1024 && i < u.length - 1) { n /= 1024; i++ }
        return (i ? n.toFixed(1) : n) + " " + u[i]
    }
    function trash() {
        var l = selected()
        if (!l.length) return
        Sys.run(["gio", "trash", "--"].concat(l), function () { load(); Ui.toast(l.length === 1 ? "Movido a la papelera" : l.length + " elementos a la papelera", false) })
    }
    function newFolder() {
        var n = "Carpeta nueva", i = 2
        while (Sys.exists(cwd + "/" + n)) n = "Carpeta nueva " + i++
        Sys.makeDir(cwd + "/" + n); load(); only(cwd + "/" + n); renameField.start()
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
    Shortcut { sequence: StandardKey.SelectAll; enabled: !search.activeFocus && !renameField.activeFocus; onActivated: win.selectAll() }
    Shortcut { sequence: "Escape"; onActivated: win.only("") }
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
                    model: [["casa", "Carpeta personal", ""], ["escritorio", "Escritorio", "/Escritorio"], ["documentos", "Documentos", "/Documentos"],
                            ["descargas", "Descargas", "/Descargas"], ["imagenes", "Imágenes", "/Imágenes"], ["musica", "Música", "/Música"],
                            ["videos", "Vídeos", "/Vídeos"], ["papelera", "Papelera", "/.local/share/Trash/files"], ["equipo", "Todo el equipo", "/"]]
                    Rectangle {
                        readonly property string p: modelData[2] === "/" ? "/" : win.home + modelData[2]
                        visible: p === "/" || Sys.exists(p)
                        Layout.fillWidth: true; Layout.preferredHeight: visible ? 32 : 0
                        radius: 8
                        color: win.cwd === p ? Ui.accent : pm.containsMouse ? Ui.cardHover : "transparent"
                        RowLayout {
                            anchors { fill: parent; leftMargin: 10 }
                            spacing: 10
                            Image { source: "qrc:/icons/fm-l-" + modelData[0] + ".svg"; sourceSize: Qt.size(18, 18) }
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
                Btn { text: "Papelera"; kind: "danger"; enabled: win.selected().length > 0; onClicked: win.trash() }
            }
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true
                radius: 12; color: Ui.card; border.color: Ui.line
                // hueco: clic quita la selección; arrastrar dibuja un recuadro que elige lo que toca
                MouseArea {
                    id: bandArea
                    anchors { fill: parent; margins: 10 }
                    property point a
                    property bool banding: false
                    onPressed: function (m) { a = Qt.point(m.x, m.y); banding = false; if (!(m.modifiers & Qt.ControlModifier)) win.only("") }
                    onPositionChanged: function (m) {
                        if (!banding && Math.abs(m.x - a.x) + Math.abs(m.y - a.y) < 6) return
                        banding = true
                        band.x = Math.min(a.x, m.x) + 10; band.y = Math.min(a.y, m.y) + 10
                        band.width = Math.abs(m.x - a.x); band.height = Math.abs(m.y - a.y)
                        var cols = Math.max(1, Math.floor(view.width / view.cellWidth)), s = {}
                        var bx = band.x - 10, by = band.y - 10 + view.contentY
                        for (var i = 0; i < view.shown.length; i++) {
                            var x = (i % cols) * view.cellWidth, y = Math.floor(i / cols) * view.cellHeight
                            if (x < bx + band.width && x + view.cellWidth - 6 > bx && y < by + band.height && y + view.cellHeight - 4 > by) s[view.shown[i].path] = true
                        }
                        win.picked = s
                    }
                    onReleased: banding = false
                    // la rueda del ratón desplaza (la vista no se arrastra con el ratón: arrastrar es seleccionar)
                    onWheel: function (w) { view.contentY = Math.max(0, Math.min(view.contentHeight - view.height, view.contentY - w.angleDelta.y)) }
                }
                // lo que llega de otra aplicación se copia a esta carpeta
                DropArea {
                    anchors.fill: parent
                    keys: ["text/uri-list"]
                    onDropped: function (drop) {
                        if (drop.source && drop.source.fileItem) { drop.accept(Qt.IgnoreAction); return }
                        var ps = win.dropPaths(drop)
                        if (ps.length) { win.transfer(ps, win.cwd, true); drop.accept(Qt.CopyAction) }
                    }
                }
                Item { id: dragProxy }
                GridView {
                    id: view
                    anchors { fill: parent; margins: 10 }
                    clip: true
                    interactive: false
                    readonly property var shown: win.items.filter(function (e) { return win.filter === "" || e.name.toLowerCase().indexOf(win.filter) >= 0 })
                    model: shown
                    cellWidth: win.grid ? 118 : width
                    cellHeight: win.grid ? 112 : 36
                    delegate: Rectangle {
                        id: cell
                        readonly property bool fileItem: true
                        readonly property bool on: win.picked[modelData.path] === true
                        width: view.cellWidth - 6; height: view.cellHeight - 4
                        radius: 9
                        color: on ? Qt.rgba(Qt.color(Ui.accent).r, Qt.color(Ui.accent).g, Qt.color(Ui.accent).b, 0.3)
                                  : (em.containsMouse || fdrop.containsDrag) ? Ui.cardHover : "transparent"
                        border.color: fdrop.containsDrag ? Ui.accent : "transparent"
                        Drag.active: em.drag.active
                        Drag.dragType: Drag.Automatic
                        Drag.supportedActions: Qt.MoveAction | Qt.CopyAction | Qt.LinkAction
                        Drag.proposedAction: Qt.MoveAction
                        Drag.mimeData: ({ "text/uri-list": win.selected().map(win.uri).join("\r\n") + "\r\n" })
                        Drag.imageSource: "qrc:/icons/" + win.icon(modelData) + ".svg"
                        Drag.onDragFinished: function () { win.load() }
                        readonly property bool isImg: win.imgs.indexOf(modelData.suffix) >= 0 && modelData.size < 20000000
                        // cuadrícula: miniatura o icono arriba y nombre debajo
                        Column {
                            visible: win.grid
                            anchors.centerIn: parent
                            spacing: 6
                            Item {
                                width: 64; height: 56; anchors.horizontalCenter: parent.horizontalCenter
                                Image { id: thumb; anchors.fill: parent; visible: parent.parent.parent.isImg && win.grid; asynchronous: true
                                        source: visible ? "file://" + modelData.path : ""; sourceSize: Qt.size(128, 112); fillMode: Image.PreserveAspectFit }
                                Image { anchors.centerIn: parent; visible: thumb.status !== Image.Ready; source: "qrc:/icons/" + win.icon(modelData) + ".svg"; sourceSize: Qt.size(56, 56) }
                            }
                            Txt { width: 104; horizontalAlignment: Text.AlignHCenter; text: modelData.name; elide: Text.ElideMiddle; font.pixelSize: Ui.fs - 1 }
                        }
                        // lista: icono, nombre, tamaño y fecha
                        RowLayout {
                            visible: !win.grid
                            anchors { fill: parent; leftMargin: 10; rightMargin: 10 }
                            spacing: 10
                            Image { source: "qrc:/icons/" + win.icon(modelData) + ".svg"; sourceSize: Qt.size(22, 22) }
                            Txt { text: modelData.name; elide: Text.ElideMiddle; Layout.fillWidth: true }
                            Txt { text: modelData.dir ? "" : win.human(modelData.size); color: Ui.muted; Layout.preferredWidth: 80; horizontalAlignment: Text.AlignRight }
                            Txt { text: Qt.formatDateTime(modelData.modified, "d MMM yyyy HH:mm"); color: Ui.muted; Layout.preferredWidth: 140 }
                        }
                        MouseArea {
                            id: em; anchors.fill: parent; hoverEnabled: true
                            drag.target: dragProxy
                            drag.threshold: 8
                            onPressed: function (m) {
                                if (m.modifiers & Qt.ControlModifier) win.toggleSel(modelData.path)
                                else if ((m.modifiers & Qt.ShiftModifier) && win.anchorIx >= 0) win.rangeSel(win.anchorIx, index)
                                else if (!cell.on) win.only(modelData.path)
                                else win.sel = modelData.path
                                if (!(m.modifiers & Qt.ShiftModifier)) win.anchorIx = index
                            }
                            onReleased: function (m) {
                                if (!drag.active && !(m.modifiers & (Qt.ControlModifier | Qt.ShiftModifier)) && win.selected().length > 1) win.only(modelData.path)
                            }
                            onDoubleClicked: win.activate(modelData)
                            onWheel: function (w) { view.contentY = Math.max(0, Math.min(view.contentHeight - view.height, view.contentY - w.angleDelta.y)) }
                        }
                        // soltar sobre una carpeta: dentro de ella (mover; con Ctrl o desde otra aplicación, copiar)
                        DropArea {
                            id: fdrop
                            anchors.fill: parent
                            enabled: modelData.dir
                            keys: ["text/uri-list"]
                            onDropped: function (d) {
                                var ps = win.dropPaths(d)
                                if (!ps.length || ps.indexOf(modelData.path) >= 0) return
                                var copy = d.proposedAction === Qt.CopyAction || !(d.source && d.source.fileItem)
                                win.transfer(ps, modelData.path, copy)
                                d.accept(copy ? Qt.CopyAction : Qt.MoveAction)
                            }
                        }
                    }
                    Txt { anchors.centerIn: parent; visible: view.count === 0; color: Ui.muted
                          text: win.filter !== "" ? "Nada coincide con «" + win.filter + "»" : "Carpeta vacía" }
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                }
                Rectangle {
                    id: band
                    visible: bandArea.banding
                    color: Qt.rgba(Qt.color(Ui.accent).r, Qt.color(Ui.accent).g, Qt.color(Ui.accent).b, 0.16)
                    border.color: Ui.accent; radius: 3
                }
            }
            RowLayout {
                Txt { text: view.count + (view.count === 1 ? " elemento" : " elementos") + (win.selected().length > 1 ? " · " + win.selected().length + " elegidos" : "") + (win.hidden ? " · con ocultos" : ""); color: Ui.muted; font.pixelSize: Ui.fs - 1; Layout.fillWidth: true }
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
