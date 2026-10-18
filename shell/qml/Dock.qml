// Dock de RROS: el Launchpad, tus aplicaciones y las abiertas, y la papelera. Los iconos crecen al pasar el ratón.
// 2D: una franja de cristal. 3D: un estante en perspectiva con brillo, canto y el reflejo de los iconos.
import QtQuick
import "."

Item {
    id: dock
    property bool vertical: Th.dockPos !== "bottom"
    property int s: Th.dockIcon
    property real zoom: Th.animations ? 1.55 : 1.25
    property real hoverPos: -1          // posición del ratón a lo largo del dock (-1: fuera)
    property bool hidden: false         // ocultación automática
    readonly property int pad: 10
    readonly property int shelf: Th.dock3d ? Math.round(s * 0.42) : 0
    // alto (o ancho) del dock en reposo y lo que ocupa con los iconos grandes
    readonly property int base: s + pad * 2
    readonly property int room: Math.ceil(s * zoom) + pad * 2 + 34
    readonly property var items: buildItems()
    implicitWidth: vertical ? room : contentLen() + 80
    implicitHeight: vertical ? contentLen() + 80 : room

    function contentLen() { var n = items.length; return n * (s + 8) + (Th.dock3d ? s : 24) + 8 }
    function buildItems() {
        var out = [], seen = {}
        if (Th.launchpadInDock) out.push({ kind: "launchpad", id: "rros-launchpad", name: "Launchpad", icon: "rros-launchpad" })
        Th.dockApps.forEach(function (id) {
            var a = Apps.get(id)
            if (!a.id) return
            seen[a.id.toLowerCase()] = 1
            out.push({ kind: "app", id: a.id, name: a.name, icon: a.icon, pinned: true })
        })
        Tk.list.forEach(function (w) {
            var a = Apps.forWindow(w.appId)
            var key = (a.id || w.appId).toLowerCase()
            if (seen[key]) return
            seen[key] = 1
            out.push({ kind: "app", id: a.id || w.appId, name: a.name || w.name, icon: a.icon || w.appId, pinned: false })
        })
        out.push({ kind: "sep" })
        out.push({ kind: "trash", id: "trash", name: "Papelera", icon: "user-trash" })
        return out
    }
    // centro de cada icono con el dock en reposo (la lupa se calcula sobre estas posiciones: no se mueven al crecer)
    function restCenter(i) {
        var len = items.length * (s + 8) - 8
        return ((vertical ? height : width) - len) / 2 + i * (s + 8) + s / 2
    }
    function sizeFor(center) {
        if (hoverPos < 0) return s
        var d = Math.abs(hoverPos - center) / (s * 1.6)
        return s * (1 + (zoom - 1) * Math.exp(-d * d))
    }
    function open(it) {
        if (it.kind === "launchpad") { Ux.toggle("launchpad"); return }
        if (it.kind === "trash") { Sh.detached(["dolphin", "trash:/"]); return }
        var ws = Tk.windowsOf(it.id)
        if (ws.length === 0) { Apps.launch(it.id); return }
        var act = ws.filter(function (w) { return w.active })
        if (act.length) Tk.toggleMinimized(act[0].row)
        else Tk.activate(ws[ws.length - 1].row)
    }

    // ---- fondo 2D ----
    Glass {
        id: plate2d
        visible: !Th.dock3d
        radius: 20
        width: dock.vertical ? dock.base : row.width + 20
        height: dock.vertical ? row.height + 20 : dock.base
        x: dock.vertical ? (Th.dockPos === "left" ? 8 : parent.width - width - 8) : (parent.width - width) / 2
        y: dock.vertical ? (parent.height - height) / 2 : parent.height - height - 8
        Behavior on width { NumberAnimation { duration: Th.anim / 2 } }
    }
    // ---- estante 3D ----
    Canvas {
        id: plate3d
        visible: Th.dock3d && !dock.vertical
        width: row.width + dock.s * 1.2
        height: dock.shelf + 8
        x: (parent.width - width) / 2
        y: parent.height - height - 6
        property color tint: Th.dark ? "#1b2142" : "#e9ecf6"
        onWidthChanged: requestPaint()
        onTintChanged: requestPaint()
        onVisibleChanged: requestPaint()
        onPaint: {
            var c = getContext("2d"), w = width, h = height, inset = dock.s * 0.45, lip = 6
            c.reset()
            // superficie del estante en perspectiva (más estrecha atrás)
            c.beginPath()
            c.moveTo(inset, 0); c.lineTo(w - inset, 0); c.lineTo(w, h - lip); c.lineTo(0, h - lip); c.closePath()
            var g = c.createLinearGradient(0, 0, 0, h - lip)
            g.addColorStop(0, Qt.rgba(1, 1, 1, Th.dark ? 0.30 : 0.75))
            g.addColorStop(0.5, Qt.rgba(1, 1, 1, Th.dark ? 0.16 : 0.55))
            g.addColorStop(1, Qt.rgba(1, 1, 1, Th.dark ? 0.10 : 0.40))
            c.fillStyle = tint; c.globalAlpha = 0.75; c.fill()
            c.globalAlpha = 1; c.fillStyle = g; c.fill()
            // borde de atrás, brillante
            c.strokeStyle = Qt.rgba(1, 1, 1, 0.7); c.lineWidth = 1
            c.beginPath(); c.moveTo(inset, 0.5); c.lineTo(w - inset, 0.5); c.stroke()
            // canto delantero
            var f = c.createLinearGradient(0, h - lip, 0, h)
            f.addColorStop(0, Qt.rgba(1, 1, 1, 0.55)); f.addColorStop(0.15, Qt.rgba(0.6, 0.65, 0.8, 0.55)); f.addColorStop(1, Qt.rgba(0.1, 0.12, 0.2, 0.7))
            c.fillStyle = f; c.fillRect(0, h - lip, w, lip)
        }
    }

    // ---- iconos ----
    Item {
        id: track
        anchors.fill: parent
        MouseArea {
            id: hover
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.NoButton
            onPositionChanged: function (m) { dock.hoverPos = dock.vertical ? m.y : m.x }
            onExited: dock.hoverPos = -1
        }
        Grid {
            id: row
            columns: dock.vertical ? 1 : dock.items.length
            rows: dock.vertical ? dock.items.length : 1
            spacing: 8
            verticalItemAlignment: Grid.AlignBottom
            horizontalItemAlignment: Th.dockPos === "right" ? Grid.AlignRight : Grid.AlignLeft
            x: dock.vertical ? (Th.dockPos === "left" ? 8 + dock.pad : parent.width - width - 8 - dock.pad) : (parent.width - width) / 2
            y: dock.vertical ? (parent.height - height) / 2 : parent.height - height - 8 - dock.pad - (Th.dock3d ? dock.shelf * 0.45 : 0)
            Repeater {
                model: dock.items
                DockItem {
                    item: modelData
                    host: dock
                    index: model.index
                }
            }
        }
    }
}
