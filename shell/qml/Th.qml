// Tema y ajustes de Lily para el escritorio: colores del tema en uso, la barra, el dock y el fondo.
// Se leen de «rros-settings api» y se vuelven a leer cuando cambian los ajustes.
pragma Singleton
import QtQuick

QtObject {
    id: t
    property var st: null
    property bool dark: st ? st.current.scheme !== "light" : true
    readonly property var fallback: ({ bg: "#140609", surface: "#220d13", text: "#f6ecdf", muted: "#b49c92",
                                       border: "#3d1a22", accent: "#c8243c", red: "#ff5a6e", green: "#7cc79a",
                                       yellow: "#e9c48a", blue: "#8fb3e0", cyan: "#efc9a6", magenta: "#e0567a" })
    property var pal: st ? st.current.palette : fallback

    // superficies: salen de la paleta del tema (en claro y en oscuro), no de colores fijos
    function rgba(c, a) { var q = Qt.color(c); return Qt.rgba(q.r, q.g, q.b, a) }
    property color glass: rgba(pal.surface, dark ? 0.80 : 0.86)
    property color glassStrong: rgba(pal.surface, dark ? 0.94 : 0.97)
    // barra de tareas translúcida sobre el desenfoque de KWin, así toma el tono del fondo; sin efectos (Lite o una
    // gráfica sin aceleración) no hay desenfoque y va casi opaca para que se lea bien
    property color barColor: rgba(pal.surface, v("ui.effects", "basic") === "off" ? (dark ? 0.92 : 0.94) : (dark ? 0.68 : 0.74))
    property color edge: dark ? Qt.rgba(1, 1, 1, 0.12) : rgba(pal.text, 0.16)
    property color hover: rgba(pal.text, dark ? 0.10 : 0.08)
    property color pressed: rgba(pal.text, dark ? 0.16 : 0.13)
    property color text: pal.text
    property color muted: pal.muted
    property color accent: pal.accent
    property color onAccent: Qt.color(pal.accent).hslLightness > 0.6 ? "#1a1a1a" : "#ffffff"
    property color tile: rgba(pal.text, dark ? 0.07 : 0.05)
    property color shadow: dark ? Qt.rgba(0, 0, 0, 0.45) : Qt.rgba(0, 0, 0, 0.18)
    property string font: "Inter"

    // ajustes
    function v(k, def) {
        if (!st) return def
        for (var i = 0; i < st.keys.length; i++)
            if (st.keys[i].key === k) return String(st.keys[i].value)
        return def
    }
    function b(k, def) { var x = v(k, def ? "true" : "false"); return x === "true" }
    function n(k, def) { var x = parseInt(v(k, String(def))); return isNaN(x) ? def : x }

    // diseño de la barra de tareas
    property string layout: v("ui.taskbar_style", "rros")
    property bool topBar: layout === "dock" || layout === "doble"
    property bool taskbarOn: layout !== "dock"
    property bool vertical: layout === "lateral"
    property bool island: layout === "isla"
    property int tbH: Math.max(36, n("ui.taskbar_size", 48))
    property bool tbCenter: v("ui.taskbar_align", "auto") === "auto" ? (layout === "rros" || layout === "isla") : v("ui.taskbar_align", "auto") === "centro"
    property bool tbLabels: b("ui.taskbar_labels", false) || layout === "clasica"
    property bool tbSearch: b("ui.taskbar_search", true) && !vertical
    // lo que la barra de tareas quita al borde de la pantalla (las ventanas maximizadas llegan justo hasta ella)
    property int tbReserve: !taskbarOn ? 0 : (island ? tbH + 10 : tbH)
    property bool dock: b("ui.dock", true) && layout === "dock"
    property bool dock3d: v("ui.dock_style", "2d") === "3d"
    property int dockIcon: n("ui.dock_icon_size", 48)
    property string dockPos: v("ui.dock_position", "bottom")
    property string dockHiding: v("ui.dock_hiding", "dodge")
    property bool launchpadInDock: b("ui.dock_launchpad", true)
    property var dockApps: {
        var s = v("ui.dock_apps", "")
        return s ? s.split(",").map(function (x) { return x.trim() }).filter(function (x) { return x !== "" })
                 : ["org.kde.dolphin", "firefox", "rros-apps", "org.kde.konsole", "org.kde.kate", "rros-settings"]
    }
    property bool barTop: v("ui.panel_position", "top") !== "bottom"
    property int barH: Math.max(28, n("ui.panel_size", 30))
    property string barStyle: v("panel.style", "flat")
    property bool taskbar: false
    property bool clock24: b("ui.clock_24h", true)
    property bool batteryPct: b("ui.show_battery_percent", true)
    property bool animations: b("ui.animations", true) && v("ui.effects", "basic") !== "off"
    property int anim: animations ? 180 : 0
    property string wallpaper: "/usr/share/wallpapers/Lily/lirio.jpg"
    property string wallpaperVideo: v("ui.wallpaper_video", "")
    property string wallpaperLive: v("ui.wallpaper_live", "off")
    property bool pauseOnBattery: b("ui.wallpaper_pause_battery", true)

    function stateDir() { return Sh.home + "/.local/state/rros" }
    function reload() {
        Sh.run(["rros-settings", "api", "GET", "/api/state"], function (r) {
            try { var s = JSON.parse(r.out); if (s && s.keys) t.st = s } catch (e) {}
        })
        var w = Sh.readFile(stateDir() + "/wallpaper").trim()
        if (w !== "" && Sh.exists(w)) wallpaper = w
    }
    function set(k, val) {
        Sh.run(["rros-settings", "api", "POST", "/api/set", "key=" + encodeURIComponent(k) + "&value=" + encodeURIComponent(val)],
               function () { reload() })
    }

    property Connections conn: Connections {
        target: Sh
        function onReloadRequested() { t.reload() }
        function onFileChanged(p) { t.reload() }
    }
    Component.onCompleted: {
        reload()
        Sh.watch(Sh.home + "/.config/rros/rros.toml")
        Sh.watch(Sh.home + "/.config/rros")
        Sh.watch(stateDir() + "/wallpaper")
        Sh.watch(stateDir())
    }
}
