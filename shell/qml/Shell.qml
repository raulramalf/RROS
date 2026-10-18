// El escritorio de RROS: cada pieza en su capa de Wayland (fondo, barra, dock, lo que se abre encima, avisos).
import QtQuick
import QtQuick.Window
import "."

Item {
    id: root

    // ---- fondo ----
    Window {
        id: wall
        visible: false
        color: Th.pal.bg
        width: Screen.width; height: Screen.height
        Wallpaper { anchors.fill: parent }
        Component.onCompleted: { Sh.layer(wall, { layer: "background", anchors: "top,bottom,left,right", exclusive: -1, scope: "rros-fondo" }); visible = true }
    }

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
        Component.onCompleted: { place(); visible = true }
        Connections { target: Th; function onBarTopChanged() { barWin.visible = false; barWin.place(); barWin.visible = true } }
        onHeightChanged: Sh.setExclusive(barWin, height)
    }

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
            function onDockChanged() { dockWin.visible = Th.dock }
            function onDockPosChanged() { dockWin.visible = false; dockWin.place(); dockWin.visible = Th.dock }
            function onDockHidingChanged() { dockWin.place() }
            function onDockIconChanged() { Sh.setExclusive(dockWin, dockWin.autoHide ? 0 : dock.base + 8) }
        }
    }

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
    }

    // ---- avisos ----
    Window {
        id: notifWin
        visible: banners.shown.length > 0
        color: "transparent"
        width: banners.width; height: Math.max(1, banners.implicitHeight)
        Banners { id: banners }
        Component.onCompleted: Sh.layer(notifWin, { layer: "overlay", anchors: "top,right", exclusive: -1, margins: [Th.barH + 10, 10, 0, 0], scope: "rros-avisos" })
    }
    Window {
        id: osdWin
        visible: osd.visible
        color: "transparent"
        width: osd.width; height: osd.height
        Osd { id: osd }
        Component.onCompleted: Sh.layer(osdWin, { layer: "overlay", anchors: "bottom", exclusive: -1, margins: [0, 0, 140, 0], scope: "rros-osd" })
    }

    Connections {
        target: Sh
        function onLaunchpadRequested() { Ux.toggle("launchpad") }
        function onControlCenterRequested() { Ux.toggle("control", wall.width - 60, Th.barH + 4) }
    }
}
