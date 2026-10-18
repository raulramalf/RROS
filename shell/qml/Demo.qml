// Modo de pruebas: todo el escritorio en una ventana (sin Wayland ni KWin), para verlo y hacer capturas.
import QtQuick
import QtQuick.Window
import "."

Window {
    id: w
    width: 1280; height: 800
    visible: true
    color: "black"
    title: "RROS Shell (demo)"
    Wallpaper { anchors.fill: parent }
    Bar { id: bar; anchors { left: parent.left; right: parent.right } y: Th.barTop ? 0 : parent.height - height; height: implicitHeight }
    Dock {
        id: dock
        visible: Th.dock
        width: vertical ? implicitWidth : Math.min(parent.width, implicitWidth)
        height: vertical ? Math.min(parent.height, implicitHeight) : implicitHeight
        x: vertical ? (Th.dockPos === "left" ? 0 : parent.width - width) : (parent.width - width) / 2
        y: vertical ? (parent.height - height) / 2 : parent.height - height
    }
    Overlay { anchors.fill: parent }
    Banners { id: banners; anchors { right: parent.right; top: parent.top; topMargin: Th.barH + 10; rightMargin: 10 } }
    Osd { anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 140 } }

    Timer {
        interval: 1500; running: true
        onTriggered: {
            // ventanas de ejemplo para el dock y la barra
            Tk.list = [{ row: 0, appId: "org.kde.dolphin", name: "Dolphin", title: "Inicio — Archivos", active: true, minimized: false },
                       { row: 1, appId: "firefox", name: "Firefox", title: "RROS", active: false, minimized: false }]
            Tk.active = Tk.list[0]
            if (openWhat === "launchpad") Ux.toggle("launchpad")
            else if (openWhat === "control") Ux.toggle("control", w.width - 120, Th.barH + 4)
            else if (openWhat === "menu") Ux.toggle("menu", 8, Th.barH + 4)
            else if (openWhat === "calendar") Ux.toggle("calendar", w.width - 90, Th.barH + 4)
            else if (openWhat === "aviso") Notifs.Notify("Tienda", 0, "rros-apps", "VLC instalada", "Ya la tienes en el Launchpad.", [], {}, 0)
            else if (openWhat === "zoom") dock.hoverPos = dock.restCenter(3)
            if (shotPath !== "") shot.start()
        }
    }
    Timer { id: shot; interval: 1200; onTriggered: { Sh.saveShot(w, shotPath); Qt.quit() } }
}
