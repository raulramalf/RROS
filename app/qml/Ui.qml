// Estado y aspecto comunes de RROS Ajustes: los colores salen del tema de RROS que esté en uso.
pragma Singleton
import QtQuick

QtObject {
    id: ui
    property var st: null                  // /api/state
    property bool loaded: st !== null
    property bool dark: st ? st.current.scheme !== "light" : true
    readonly property var fallback: ({ bg: "#0b1020", surface: "#141c38", text: "#e6ecff", muted: "#8a96bd",
                                       border: "#26305a", accent: "#7c5cff", red: "#ff6b81", green: "#4ade80",
                                       yellow: "#fbbf24", blue: "#60a5fa", cyan: "#22d3ee", magenta: "#c084fc" })
    property var pal: st ? st.current.palette : fallback

    // superficies
    property color window: pal.bg
    property color sidebar: Qt.tint(pal.bg, dark ? "#0affffff" : "#08000000")
    property color card: dark ? Qt.tint(pal.surface, "#08ffffff") : Qt.lighter(pal.surface, 1.02)
    property color cardHover: Qt.tint(card, dark ? "#10ffffff" : "#0a000000")
    property color field: dark ? Qt.tint(pal.bg, "#14ffffff") : Qt.darker(pal.surface, 1.04)
    property color line: Qt.rgba(textColor.r, textColor.g, textColor.b, dark ? 0.09 : 0.11)
    property color textColor: pal.text
    property color muted: pal.muted
    property color accent: pal.accent
    property color accentText: luminance(pal.accent) > 0.6 ? "#101010" : "#ffffff"
    property color danger: pal.red
    property color ok: pal.green
    property color warn: pal.yellow
    property color track: dark ? Qt.tint(pal.bg, "#26ffffff") : Qt.darker(pal.surface, 1.15)

    // tipografía y medidas
    property string font: "Inter"
    property int fs: 13
    property int radius: 12
    property int rowH: 52

    signal toast(string text, bool error)
    signal stateChanged()

    function luminance(c) {
        var q = Qt.color(c)
        return 0.299 * q.r + 0.587 * q.g + 0.114 * q.b
    }

    function reload(cb) {
        Sys.api("GET", "/api/state", {}, function (s) {
            if (s && s.keys) {
                ui.st = s
                ui.stateChanged()
            }
            if (cb) cb(s)
        })
    }

    function key(k) {
        if (!st) return null
        for (var i = 0; i < st.keys.length; i++)
            if (st.keys[i].key === k) return st.keys[i]
        return null
    }
    function v(k, def) { var x = key(k); return x && x.value !== undefined && x.value !== null ? String(x.value) : (def === undefined ? "" : def) }
    function b(k) { return v(k) === "true" }
    function n(k, def) { var x = parseFloat(v(k)); return isNaN(x) ? (def || 0) : x }

    // cambia una clave: las del usuario al momento; las del sistema piden la contraseña de administrador
    function setKey(k, value, cb) {
        var kk = key(k)
        var path = kk && kk.scope === "system" ? "/api/sys_set" : "/api/set"
        Sys.api("POST", path, { key: k, value: String(value) }, function (r) {
            if (!r || !r.ok) toast(r && r.error ? r.error : "No se pudo guardar", true)
            reload(function () { if (cb) cb(r) })
        })
    }
    function unsetKey(k, cb) {
        var kk = key(k)
        Sys.api("POST", kk && kk.scope === "system" ? "/api/sys_unset" : "/api/unset", { key: k }, function (r) {
            if (!r || !r.ok) toast(r && r.error ? r.error : "No se pudo restablecer", true)
            reload(function () { if (cb) cb(r) })
        })
    }

    function api(m, p, body, cb) { Sys.api(m, p, body || {}, cb || function () {}) }
    function run(argv, cb) { Sys.run(argv, cb || function () {}) }

    // «nmcli -t»: campos separados por ':' con '\:' escapado
    function splitT(line) {
        var out = [], cur = "", esc = false
        for (var i = 0; i < line.length; i++) {
            var c = line[i]
            if (esc) { cur += c; esc = false }
            else if (c === "\\") esc = true
            else if (c === ":") { out.push(cur); cur = "" }
            else cur += c
        }
        out.push(cur)
        return out
    }
    function lines(t) { return t ? t.split("\n").filter(function (l) { return l.length > 0 }) : [] }
    function human(bytes) {
        var u = ["B", "KB", "MB", "GB", "TB"], i = 0, x = bytes
        while (x >= 1000 && i < u.length - 1) { x /= 1000; i++ }
        return (i > 1 ? x.toFixed(1) : Math.round(x)) + " " + u[i]
    }
}
