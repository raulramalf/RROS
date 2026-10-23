// Qué está abierto encima del escritorio (menús, centro de control, Launchpad) y dónde.
pragma Singleton
import QtQuick

QtObject {
    property string open: ""          // "", "menu", "control", "calendar", "notifs", "launchpad", "start", "dockmenu", "desktopmenu", "filemenu"
    property real ax: 0               // punto de anclaje del menú abierto
    property real ay: 0
    property var data: null
    property string demoSearch: ""          // solo en el modo demo: escribe en el buscador del inicio           // lo que necesite el menú (la app del dock, etc.)
    function toggle(what, x, y, d) {
        if (open === what) { open = ""; return }
        ax = x || 0; ay = y || 0; data = d || null; open = what
    }
    function close() { open = "" }
    // lo que se elige en el menú de un archivo del escritorio (lo hace DesktopIcons): open, trash, copypath, show, newfolder
    signal fileAction(string action, var paths)
}
