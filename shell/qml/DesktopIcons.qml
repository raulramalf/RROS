// Los archivos de la carpeta Escritorio sobre el fondo, en columnas desde arriba a la izquierda (como en Windows).
// Clic: elegir; Ctrl+clic: añadir o quitar; Mayús+clic: un tramo; arrastrar en un hueco: recuadro de selección.
// Doble clic: abrir. Arrastrar iconos: a una carpeta del escritorio (mover; con Ctrl, copiar), a la papelera del
// dock o a otra aplicación (Archivos, el navegador...). Lo que se suelta desde otra aplicación se copia aquí.
import QtQuick
import "."

Item {
    id: dk
    // LILY_DESKTOP (pruebas): otra carpeta como escritorio, también en el modo demo
    readonly property string testDir: Sh.env("LILY_DESKTOP")
    property string dir: testDir !== "" ? testDir : Sh.desktopDir()
    property bool show: true
    property var items: []
    property var sel: ({})          // ruta -> true
    property int anchorIx: -1       // para Mayús+clic
    readonly property int cw: 92
    readonly property int ch: 100
    readonly property int padTop: 12 + (Th.topBar ? Th.barH : 0)
    readonly property int padLeft: 12 + (Th.vertical ? Th.tbH : 0)
    readonly property int rows: Math.max(1, Math.floor((height - padTop - 12 - (Th.vertical ? 0 : Th.tbReserve) - (Th.dock ? Th.dockIcon + 40 : 0)) / ch))
    signal backgroundMenu(real x, real y)

    function load() { items = demoMode && testDir === "" ? demoItems() : Sh.entries(dir); var s = {}; for (var k in sel) if (Sh.exists(k)) s[k] = true; sel = s }
    function demoItems() {
        return [{ path: "/demo/Proyectos", name: "Proyectos", dir: true, suffix: "" }, { path: "/demo/Fotos del viaje", name: "Fotos del viaje", dir: true, suffix: "" },
                { path: "/demo/informe.pdf", name: "informe.pdf", dir: false, suffix: "pdf" }, { path: "/demo/notas.txt", name: "notas.txt", dir: false, suffix: "txt" },
                { path: "/demo/presupuesto.ods", name: "presupuesto.ods", dir: false, suffix: "ods" }, { path: "/demo/cancion.mp3", name: "cancion.mp3", dir: false, suffix: "mp3" }]
    }
    function cellX(i) { return padLeft + Math.floor(i / rows) * cw }
    function cellY(i) { return padTop + (i % rows) * ch }
    function selected() { var out = []; for (var k in sel) out.push(k); return out }
    function only(p) { var s = {}; s[p] = true; sel = s }
    function clear() { sel = ({}) }
    function toggle(p) { var s = Object.assign({}, sel); if (s[p]) delete s[p]; else s[p] = true; sel = s }
    function range(a, b) { var s = Object.assign({}, sel); for (var i = Math.min(a, b); i <= Math.max(a, b); i++) s[items[i].path] = true; sel = s }
    function uri(p) { return "file://" + encodeURI(p).replace(/#/g, "%23").replace(/\?/g, "%3F") }
    function open(e) {
        if (e.suffix === "desktop") Sh.detached(["gio", "launch", e.path])
        else Sh.detached(["xdg-open", e.path])
    }
    function trash(paths) { if (paths.length) Sh.run(["gio", "trash", "--"].concat(paths), function () { dk.load() }) }
    // mover o copiar dentro de una carpeta: sin sobrescribir (-n) y con «--» por si un nombre empieza por «-»
    function transfer(paths, dest, copy) {
        paths = paths.filter(function (p) { return p !== dest && p.substring(0, p.lastIndexOf("/")) !== dest && dest.indexOf(p + "/") !== 0 })
        if (!paths.length) return
        Sh.run(copy ? ["cp", "-r", "-n", "-t", dest, "--"].concat(paths) : ["mv", "-n", "-t", dest, "--"].concat(paths), function () { dk.load() })
    }
    function dropPaths(drop) {
        var out = []
        if (drop.hasUrls) for (var i = 0; i < drop.urls.length; i++) {
            var u = String(drop.urls[i])
            if (u.indexOf("file://") === 0) out.push(decodeURIComponent(u.substring(7)))
        }
        return out
    }
    function icon(e) {
        if (e.suffix === "desktop" && e.icon) return "image://icon/" + e.icon
        var x = e.suffix
        if (e.dir) return "qrc:/icons/fm-carpeta.svg"
        if (["png", "jpg", "jpeg", "webp", "gif", "bmp", "svg"].indexOf(x) >= 0) return "qrc:/icons/fm-imagen.svg"
        if (["mp4", "mkv", "webm", "avi", "mov"].indexOf(x) >= 0) return "qrc:/icons/fm-video.svg"
        if (["mp3", "ogg", "flac", "wav", "m4a", "opus"].indexOf(x) >= 0) return "qrc:/icons/fm-audio.svg"
        if (["zip", "tar", "gz", "xz", "zst", "7z", "rar", "bz2"].indexOf(x) >= 0) return "qrc:/icons/fm-comprimido.svg"
        if (x === "pdf") return "qrc:/icons/fm-pdf.svg"
        if (["ods", "xlsx", "xls", "csv"].indexOf(x) >= 0) return "qrc:/icons/fm-hoja.svg"
        if (["sh", "py", "js", "c", "h", "cpp", "qml", "json", "toml", "yml", "yaml", "html", "css", "rs", "go"].indexOf(x) >= 0) return "qrc:/icons/fm-codigo.svg"
        if (["appimage", "run", "bin", "exe"].indexOf(x) >= 0) return "qrc:/icons/fm-ejecutable.svg"
        return "qrc:/icons/fm-texto.svg"
    }

    Component.onCompleted: { if (!demoMode || testDir !== "") { Sh.run(["mkdir", "-p", "--", dir], function () { Sh.watch(dk.dir); dk.load() }) } else load() }
    Connections { target: Sh; function onFileChanged(p) { if (p === dk.dir) dk.load() } }
    Connections { target: Ux; function onFileAction(a, paths) {
        if (a === "open") paths.forEach(function (p) { var e = dk.items.filter(function (x) { return x.path === p })[0]; if (e) dk.open(e) })
        else if (a === "trash") dk.trash(paths)
        else if (a === "copypath") Sh.copy(paths.join("\n"))
        else if (a === "show") Sh.detached(["dolphin", "--select"].concat(paths))
        else if (a === "newfolder") {
            var n = "Carpeta nueva", i = 2
            while (Sh.exists(dk.dir + "/" + n)) n = "Carpeta nueva " + i++
            Sh.run(["mkdir", "--", dk.dir + "/" + n], function () { dk.load(); dk.only(dk.dir + "/" + n) })
        }
    } }

    // fondo: clic en un hueco (quita la selección), clic derecho (menú del escritorio) y recuadro de selección
    MouseArea {
        id: bg
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        property point a
        property bool banding: false
        onPressed: function (m) {
            Ux.close()
            if (m.button === Qt.RightButton) return
            a = Qt.point(m.x, m.y); banding = false
            if (!(m.modifiers & Qt.ControlModifier)) dk.clear()
        }
        onPositionChanged: function (m) {
            if (!(m.buttons & Qt.LeftButton)) return
            if (!banding && Math.abs(m.x - a.x) + Math.abs(m.y - a.y) < 6) return
            banding = true
            band.x = Math.min(a.x, m.x); band.y = Math.min(a.y, m.y)
            band.width = Math.abs(m.x - a.x); band.height = Math.abs(m.y - a.y)
            var s = {}
            for (var i = 0; i < dk.items.length; i++) {
                var x = dk.cellX(i) + 6, y = dk.cellY(i) + 4, w = dk.cw - 12, h = dk.ch - 8
                if (x < band.x + band.width && x + w > band.x && y < band.y + band.height && y + h > band.y) s[dk.items[i].path] = true
            }
            dk.sel = s
        }
        onReleased: banding = false
        onClicked: function (m) { if (m.button === Qt.RightButton) dk.backgroundMenu(m.x, m.y) }
    }
    Rectangle {
        id: band
        visible: bg.banding
        color: Qt.rgba(Qt.color(Th.accent).r, Qt.color(Th.accent).g, Qt.color(Th.accent).b, 0.18)
        border.color: Th.accent; border.width: 1; radius: 3
    }

    // lo que llega de otra aplicación se copia al escritorio
    DropArea {
        anchors.fill: parent
        keys: ["text/uri-list"]
        onDropped: function (drop) {
            if (drop.source && drop.source.desktopIcon) { drop.accept(Qt.IgnoreAction); return }
            var ps = dk.dropPaths(drop)
            if (!ps.length) return
            dk.transfer(ps, dk.dir, true)
            drop.accept(Qt.CopyAction)
        }
    }

    Item { id: dragProxy }

    Repeater {
        model: dk.show ? dk.items : []
        delegate: Item {
            id: ic
            readonly property bool desktopIcon: true
            readonly property bool on: dk.sel[modelData.path] === true
            readonly property bool isImg: ["png", "jpg", "jpeg", "webp", "gif", "bmp"].indexOf(modelData.suffix) >= 0 && modelData.size < 30000000
            x: dk.cellX(index); y: dk.cellY(index)
            width: dk.cw; height: dk.ch
            Drag.active: ma.drag.active
            Drag.dragType: Drag.Automatic
            Drag.supportedActions: Qt.MoveAction | Qt.CopyAction | Qt.LinkAction
            Drag.proposedAction: Qt.MoveAction
            Drag.mimeData: ({ "text/uri-list": dk.selected().map(dk.uri).join("\r\n") + "\r\n" })
            Drag.imageSource: icoImg.source
            Drag.onDragFinished: function () { dk.load() }

            Rectangle {
                anchors { fill: parent; margins: 4 }
                radius: 10
                color: ic.on ? Qt.rgba(Qt.color(Th.accent).r, Qt.color(Th.accent).g, Qt.color(Th.accent).b, 0.32)
                     : (ma.containsMouse || drop.containsDrag) ? Qt.rgba(1, 1, 1, 0.12) : "transparent"
                border.color: ic.on || drop.containsDrag ? Qt.rgba(Qt.color(Th.accent).r, Qt.color(Th.accent).g, Qt.color(Th.accent).b, 0.7) : "transparent"
            }
            Item {
                id: pic
                width: 52; height: 52
                anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; topMargin: 8 }
                Image { id: thumb; anchors.fill: parent; visible: ic.isImg && status === Image.Ready; asynchronous: true
                        source: ic.isImg && (!demoMode || dk.testDir !== "") ? "file://" + modelData.path : ""; sourceSize: Qt.size(104, 104); fillMode: Image.PreserveAspectFit }
                Image { id: icoImg; anchors.fill: parent; visible: !thumb.visible; source: dk.icon(modelData); sourceSize: Qt.size(104, 104); smooth: true }
            }
            Text {
                anchors { top: pic.bottom; topMargin: 4; horizontalCenter: parent.horizontalCenter }
                width: dk.cw - 10
                horizontalAlignment: Text.AlignHCenter
                text: modelData.title || (modelData.suffix === "desktop" ? modelData.name.replace(/\.desktop$/, "") : modelData.name)
                color: "white"; font.pixelSize: 12; font.family: Th.font
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere; maximumLineCount: ic.on ? 4 : 2; elide: Text.ElideRight
                style: Text.Raised; styleColor: Qt.rgba(0, 0, 0, 0.75)
            }
            MouseArea {
                id: ma
                anchors { fill: parent; margins: 4 }
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                drag.target: dragProxy
                drag.threshold: 8
                onPressed: function (m) {
                    Ux.close()
                    if (m.modifiers & Qt.ControlModifier) { if (m.button === Qt.LeftButton) dk.toggle(modelData.path) }
                    else if ((m.modifiers & Qt.ShiftModifier) && dk.anchorIx >= 0) dk.range(dk.anchorIx, index)
                    else if (!ic.on) dk.only(modelData.path)
                    if (!(m.modifiers & Qt.ShiftModifier)) dk.anchorIx = index
                }
                onReleased: function (m) {
                    // clic sin arrastrar sobre uno ya elegido (de varios): queda solo ese
                    if (!drag.active && m.button === Qt.LeftButton && !(m.modifiers & (Qt.ControlModifier | Qt.ShiftModifier)) && dk.selected().length > 1)
                        dk.only(modelData.path)
                }
                onClicked: function (m) {
                    if (m.button === Qt.RightButton)
                        Ux.toggle("filemenu", ic.x + m.x, ic.y + m.y, { paths: dk.selected(), dir: modelData.dir })
                }
                onDoubleClicked: function (m) { if (m.button === Qt.LeftButton) dk.open(modelData) }
            }
            // soltar sobre una carpeta: dentro de ella (mover; con Ctrl, copiar)
            DropArea {
                id: drop
                anchors.fill: parent
                enabled: modelData.dir
                keys: ["text/uri-list"]
                onDropped: function (d) {
                    var ps = dk.dropPaths(d)
                    if (!ps.length || ps.indexOf(modelData.path) >= 0) return
                    var copy = d.proposedAction === Qt.CopyAction || !(d.source && d.source.desktopIcon)
                    dk.transfer(ps, modelData.path, copy)
                    d.accept(copy ? Qt.CopyAction : Qt.MoveAction)
                }
            }
        }
    }
}
