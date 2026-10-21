// El escritorio de Lily: cada pieza en su capa de Wayland (fondo, barra, dock, lo que se abre encima, avisos).
import QtQml
import QtQuick
import QtQuick.Window
import "."

QtObject {
    id: root
    // las ventanas van en una lista y no dentro de un Item: un Window dentro de un Item toma ese Item como padre
    // y Qt no lo muestra hasta que el padre tenga ventana (aquí nunca la tendría)
    property list<QtObject> partes: [

    // ---- fondo ----
    Window {
        id: wall
        visible: false
        color: Th.pal.bg
        width: Screen.width; height: Screen.height
        Wallpaper { anchors.fill: parent }
        Component.onCompleted: { Sh.layer(wall, { layer: "background", anchors: "top,bottom,left,right", exclusive: -1, scope: "rros-fondo" }); visible = true }
    },

    // ---- barra ----
    Window {
        id: barWin
        visible: false
        color: "transparent"
        width: Screen.width
        height: bar.implicitHeight
        Bar { id: bar; anchors.fill: parent }
        function place() {
            Sh.layer(barWin, { layer: "top", anchors: Th.barTop ? "top,left,right" : "bottom,left,right", exclusive: bar.implicitHeight, scope: "rros-barra" })
            Sh.blurBehind(barWin, true, [])
        }
        Component.onCompleted: { place(); visible = Th.topBar }
        Connections {
            target: Th
            function onBarTopChanged() { barWin.visible = false; barWin.place(); barWin.visible = Th.topBar }
            function onTopBarChanged() { barWin.visible = Th.topBar }
        }
        onHeightChanged: Sh.setExclusive(barWin, height)
    },

    // ---- barra de tareas (abajo de borde a borde, isla flotante o lateral): siempre visible y con su franja
    // reservada, así las ventanas maximizadas llegan justo hasta ella y nada queda debajo ----
    Window {
        id: tbWin
        visible: false
        color: "transparent"
        width: Th.vertical ? taskbar.implicitWidth : Screen.width
        height: Th.vertical ? Screen.height : taskbar.implicitHeight
        Taskbar { id: taskbar; anchors.fill: parent }
        function place() {
            Sh.layer(tbWin, { layer: "top", anchors: Th.vertical ? "left,top,bottom" : "bottom,left,right",
                              exclusive: Th.vertical ? Th.tbH : Th.tbReserve, keyboard: "ondemand", scope: "rros-tareas" })
            Sh.blurBehind(tbWin, true, [])
        }
        Component.onCompleted: { place(); visible = Th.taskbarOn }
        Connections {
            target: Th
            function onLayoutChanged() { tbWin.visible = false; tbWin.place(); tbWin.visible = Th.taskbarOn }
            function onTbReserveChanged() { Sh.setExclusive(tbWin, Th.vertical ? Th.tbH : Th.tbReserve) }
        }
    },

    // ---- dock ----
    Window {
        id: dockWin
        visible: false
        color: "transparent"
        width: dock.vertical ? dock.implicitWidth : Math.min(Screen.width, dock.implicitWidth)
        height: dock.vertical ? Math.min(Screen.height, dock.implicitHeight) : dock.implicitHeight
        property bool autoHide: Th.dockHiding === "auto"
        property bool shown: !autoHide || hov.hovered || Ux.open === "dockmenu"
        Item {
            anchors.fill: parent
            HoverHandler { id: hov }
            Dock {
                id: dock
                anchors.fill: parent
                opacity: dockWin.shown ? 1 : 0
                transform: Translate {
                    y: dock.vertical ? 0 : (dockWin.shown ? 0 : dock.base)
                    x: !dock.vertical ? 0 : (dockWin.shown ? 0 : (Th.dockPos === "left" ? -dock.base : dock.base))
                    Behavior on y { NumberAnimation { duration: Th.anim } }
                    Behavior on x { NumberAnimation { duration: Th.anim } }
                }
                Behavior on opacity { NumberAnimation { duration: Th.anim } }
            }
        }
        // solo la franja del dock recibe el ratón (encima, las ventanas de debajo siguen respondiendo)
        function updateInput() {
            var b = dock.base + 16, r
            if (!dockWin.shown) b = 4
            if (dock.vertical) r = { x: Th.dockPos === "left" ? 0 : width - b, y: 0, width: b, height: height }
            else r = { x: 0, y: height - b - (hov.hovered ? dock.room - dock.base : 0), width: width, height: b + (hov.hovered ? dock.room - dock.base : 0) }
            Sh.inputRegion(dockWin, [r])
        }
        function place() {
            var a = Th.dockPos === "left" ? "left" : Th.dockPos === "right" ? "right" : "bottom"
            Sh.layer(dockWin, { layer: "top", anchors: a, exclusive: autoHide ? 0 : dock.base + 8, scope: "rros-dock" })
        }
        Component.onCompleted: { place(); visible = Th.dock; updateInput() }
        onShownChanged: updateInput()
        onWidthChanged: updateInput()
        Connections { target: hov; function onHoveredChanged() { dockWin.updateInput() } }
        Connections {
            target: Th
            function onDockChanged() { dockWin.place(); dockWin.visible = Th.dock }
            function onDockPosChanged() { dockWin.visible = false; dockWin.place(); dockWin.visible = Th.dock }
            function onDockHidingChanged() { dockWin.place() }
            function onDockIconChanged() { Sh.setExclusive(dockWin, dockWin.autoHide ? 0 : dock.base + 8) }
        }
    },

    // ---- lo que se abre encima (menús, centro de control, Launchpad) ----
    Window {
        id: overWin
        visible: false
        color: "transparent"
        width: Screen.width; height: Screen.height
        Overlay { anchors.fill: parent }
        Connections {
            target: Ux
            function onOpenChanged() {
                if (Ux.open === "") { overWin.visible = false; return }
                Sh.layer(overWin, { layer: "overlay", anchors: "top,bottom,left,right", exclusive: -1,
                                    keyboard: Ux.open === "launchpad" ? "exclusive" : "ondemand", scope: "rros-encima" })
                Sh.blurBehind(overWin, Ux.open === "launchpad", [])
                overWin.visible = true
                overWin.requestActivate()
            }
        }
    },

    // ---- avisos ----
    Window {
        id: notifWin
        visible: banners.shown.length > 0
        color: "transparent"
        width: banners.width; height: Math.max(1, banners.implicitHeight)
        Banners { id: banners }
        Component.onCompleted: Sh.layer(notifWin, { layer: "overlay", anchors: "top,right", exclusive: -1, margins: [Th.topBar ? Th.barH + 10 : 10, 10, 0, 0], scope: "rros-avisos" })
    },
    Window {
        id: osdWin
        visible: osd.visible
        color: "transparent"
        width: osd.width; height: osd.height
        Osd { id: osd }
        Component.onCompleted: Sh.layer(osdWin, { layer: "overlay", anchors: "bottom", exclusive: -1, margins: [0, 0, 140, 0], scope: "rros-osd" })
    },

    Connections {
        target: Sh
        // la tecla Windows: el Launchpad con el dock; el menú de inicio con la barra de tareas
        function onFocusTaskbarRequested() { tbWin.requestActivate() }
        function onDndRequested(on) { Notifs.doNotDisturb = on }
        function onLaunchpadRequested() { if (Th.dock) Ux.toggle("launchpad"); else Ux.toggle("start", Th.vertical ? Th.tbH + 8 : 6, Th.vertical ? 8 : -1) }
        function onControlCenterRequested() { Ux.toggle("control", wall.width - 60, Th.barH + 4) }
    }
    ]
}
