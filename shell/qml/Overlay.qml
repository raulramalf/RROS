// Lo que se abre encima del escritorio: menú Lily, centro de control, calendario, Launchpad y menús del dock y del fondo.
// Un clic fuera lo cierra.
import QtQuick
import "."

Item {
    id: ov
    function clampX(x, w) { return Math.max(8, Math.min(x, width - w - 8)) }
    // con la barra abajo los menús se abren encima de ella; con la barra lateral, a su derecha
    function menuTop(h) { return Ux.ay >= 0 ? Math.min(Ux.ay, height - h - 8) : height - h - Th.tbReserve - 8 }
    function menuX(w, right) { return Th.vertical ? Ux.ax : clampX(right ? Ux.ax - w : Ux.ax, w) }

    MouseArea {
        anchors.fill: parent
        enabled: Ux.open !== "" && Ux.open !== "launchpad"
        acceptedButtons: Qt.AllButtons
        onClicked: Ux.close()
    }
    Launchpad { anchors.fill: parent; visible: Ux.open === "launchpad" }
    StartMenu {
        visible: Ux.open === "start"
        x: ov.menuX(width, false); y: ov.menuTop(height)
    }

    MenuList {
        id: rmenu
        visible: Ux.open === "menu"
        x: ov.clampX(Ux.ax, width); y: ov.menuTop(height)
        entries: [["Acerca de este equipo", "rros-ajustes --pagina general"], ["-"],
                  ["Ajustes…", "rros-ajustes"], ["Tienda…", "rros-ajustes --pagina tienda"], ["Actualizaciones…", "rros-ajustes --pagina actualizaciones"]]
                 .concat(Sh.live ? [["Instalar Lily…", "rros-ajustes --pagina instalar"]] : [])
                 .concat([["-"], ["Forzar salida…", "qdbus6 org.kde.KWin /KWin killWindow"], ["Monitor del sistema", "plasma-systemmonitor"], ["-"],
                           ["Reposo", "systemctl suspend"], ["Reiniciar…", "qdbus6 org.kde.LogoutPrompt /LogoutPrompt promptReboot"],
                           ["Apagar…", "qdbus6 org.kde.LogoutPrompt /LogoutPrompt promptShutDown"], ["-"],
                           ["Bloquear la pantalla", "loginctl lock-session"], ["Cerrar sesión…", "qdbus6 org.kde.LogoutPrompt /LogoutPrompt promptLogout"]])
        onPicked: function (a) { Sh.detached(a.split(" ")) }
    }
    ControlCenter {
        visible: Ux.open === "control"
        x: ov.menuX(width, true); y: ov.menuTop(height)
    }
    CalendarPop {
        visible: Ux.open === "calendar"
        x: ov.menuX(width, true); y: ov.menuTop(height)
    }
    MenuList {
        id: dmenu
        visible: Ux.open === "dockmenu"
        property var it: Ux.data || ({})
        property var wins: it.id ? Tk.windowsOf(it.id) : []
        x: Th.vertical ? Ux.ax : ov.clampX(Ux.ax - width / 2, width); y: Th.vertical ? Ux.ay : Ux.ay - height - 14
        entries: it.kind === "trash" ? [["Abrir la papelera", "open"], ["Vaciar la papelera", "empty"]]
               : it.kind === "launchpad" ? [["Abrir el Launchpad", "open"]]
               : [[wins.length ? "Nueva ventana" : "Abrir", "new"], [it.pinned ? (Th.dock ? "Quitar del dock" : "Desanclar de la barra") : (Th.dock ? "Mantener en el dock" : "Anclar a la barra"), "pin"]]
                 .concat(wins.length ? [["-"], ["Cerrar", "close"]] : [])
        onPicked: function (a) {
            if (a === "open" && it.kind === "launchpad") Ux.toggle("launchpad")
            else if (a === "open") Sh.detached(["dolphin", "trash:/"])
            else if (a === "empty") Sh.detached(["kioclient", "empty", "trash:/"])
            else if (a === "new") Apps.launch(it.id)
            else if (a === "close") wins.forEach(function (w) { Tk.close(w.row) })
            else if (a === "pin") {
                var l = Th.dockApps.slice(), i = l.indexOf(it.id)
                if (i >= 0) l.splice(i, 1); else l.push(it.id)
                Th.set("ui.dock_apps", l.join(","))
            }
        }
    }
    MenuList {
        visible: Ux.open === "desktopmenu"
        x: ov.clampX(Ux.ax, width); y: Math.min(Ux.ay, ov.height - height - 8)
        entries: [["Cambiar el fondo…", "rros-ajustes --pagina apariencia"], ["Barra de tareas y escritorio…", "rros-ajustes --pagina escritorio"],
                  ["Pantallas…", "rros-ajustes --pagina pantalla"], ["-"], ["Abrir una terminal", "konsole"], ["Archivos", "dolphin"]]
        onPicked: function (a) { Sh.detached(a.split(" ")) }
    }
}
