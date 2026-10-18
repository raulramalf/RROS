// Tema y ajustes de RROS para el escritorio: colores del tema en uso, la barra, el dock y el fondo.
// Se leen de «rros-settings api» y se vuelven a leer cuando cambian los ajustes.
pragma Singleton
import QtQuick

QtObject {
    id: t
    property var st: null
    property bool dark: st ? st.current.scheme !== "light" : true
    readonly property var fallback: ({ bg: "#0b1020", surface: "#141c38", text: "#e6ecff", muted: "#8a96bd",
                                       border: "#26305a", accent: "#7c5cff", red: "#ff6b81", green: "#4ade80",
                                       yellow: "#fbbf24", blue: "#60a5fa", cyan: "#22d3ee", magenta: "#c084fc" })
    property var pal: st ? st.current.palette : fallback

    // cristal: el color del tema, translúcido
    property color glass: dark ? Qt.rgba(0.07, 0.08, 0.14, 0.62) : Qt.rgba(0.97, 0.97, 1.0, 0.62)
    property color glassStrong: dark ? Qt.rgba(0.08, 0.09, 0.16, 0.86) : Qt.rgba(0.98, 0.98, 1.0, 0.88)
    property color edge: dark ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(0, 0, 0, 0.10)
    property color hover: dark ? Qt.rgba(1, 1, 1, 0.10) : Qt.rgba(0, 0, 0, 0.07)
    property color text: dark ? "#f2f4ff" : "#14172a"
    property color muted: dark ? Qt.rgba(1, 1, 1, 0.62) : Qt.rgba(0, 0, 0, 0.55)
    property color accent: pal.accent
    property color tile: dark ? Qt.rgba(1, 1, 1, 0.08) : Qt.rgba(0, 0, 0, 0.05)
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

    property bool dock: b("ui.dock", true) && !taskbar
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
    property int barH: Math.max(24, n("ui.panel_size", 28))
    property string barStyle: v("panel.style", "flat")
    property bool taskbar: b("panel.taskbar", false)
    property bool clock24: b("ui.clock_24h", true)
    property bool batteryPct: b("ui.show_battery_percent", true)
    property bool animations: b("ui.animations", true) && v("ui.effects", "basic") !== "off"
    property int anim: animations ? 180 : 0
    property string wallpaper: "/usr/share/wallpapers/RROS/contents/images/1920x1080.png"

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
