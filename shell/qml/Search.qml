// Búsqueda global del menú de inicio (como Spotlight o la de Windows): aplicaciones, ajustes, cálculos, archivos de tu
// carpeta personal y, al final, la web. Cada resultado: { kind, title, sub, icon, run(): acción }.
pragma Singleton
import QtQuick

QtObject {
    id: s
    property string q: ""
    property var files: []
    property int seq: 0
    // páginas de Ajustes y palabras con las que se encuentran
    readonly property var settings: [
        ["apariencia", "Apariencia", "tema oscuro claro color acento fondo pantalla letra fuente cursor puntero animado video"],
        ["escritorio", "Barra de tareas", "barra tareas dock inicio iconos ventanas"],
        ["red", "Red e Internet", "wifi red internet ethernet vpn proxy"],
        ["bluetooth", "Bluetooth", "bluetooth auriculares"], ["sonido", "Sonido", "sonido volumen altavoz micro audio"],
        ["pantalla", "Pantallas", "pantalla monitor resolución brillo luz nocturna"], ["raton", "Ratón y panel táctil", "ratón touchpad"],
        ["teclado", "Teclado y atajos", "teclado atajos idioma distribución"], ["energia", "Batería y energía", "batería energía suspender"],
        ["dispositivos", "Dispositivos", "usb discos impresora"], ["fecha", "Fecha, hora e idioma", "fecha hora reloj idioma zona"],
        ["accesibilidad", "Accesibilidad", "accesibilidad lector pantalla zoom contraste"],
        ["seguridad", "Seguridad y privacidad", "seguridad antivirus virus cortafuegos contraseña privacidad usb"],
        ["copias", "Copias y restauración", "copia seguridad backup restaurar instantánea volver atrás"],
        ["tienda", "Tienda", "tienda instalar aplicaciones programas"], ["actualizaciones", "Actualizaciones", "actualizar actualizaciones"]]

    // calculadora: solo números, operadores, paréntesis y unas pocas funciones (nada de código)
    function calc(t) {
        var e = t.replace(/,/g, ".").replace(/×/g, "*").replace(/÷/g, "/").replace(/\^/g, "**").trim()
        if (!/[0-9]/.test(e) || !/[-+*\/%()]|sqrt|pi/.test(e)) return null
        if (!/^(?:[0-9.\s+\-*\/%()]|\*\*|sqrt|pi|sin|cos|tan|log)*$/.test(e)) return null
        e = e.replace(/sqrt/g, "Math.sqrt").replace(/\bpi\b/g, "Math.PI").replace(/\b(sin|cos|tan|log)\b/g, "Math.$1")
        try { var v = Function("return (" + e + ")")(); return typeof v === "number" && isFinite(v) ? v : null } catch (x) { return null }
    }
    function fmt(v) { return Number.isInteger(v) ? String(v) : v.toLocaleString(Qt.locale("es_ES"), "f", 6).replace(/0+$/, "").replace(/,$/, "") }

    onQChanged: {
        files = []
        if (q.length < 3) return
        var my = ++seq
        // archivos y carpetas de tu carpeta personal (sin las ocultas), los 8 primeros
        Sh.run(["find", Sh.home, "-maxdepth", "6", "-not", "-path", "*/.*", "-iname", "*" + q.replace(/[*?\[\]]/g, "") + "*"], function (r) {
            if (my !== s.seq) return
            s.files = r.out.split("\n").filter(function (l) { return l !== "" }).slice(0, 8)
        })
    }
    function results(apps) {
        var out = [], ql = q.toLowerCase()
        if (q === "") return out
        var v = calc(q)
        if (v !== null) out.push({ kind: "Cálculo", title: "= " + fmt(v), sub: q, icon: "accessories-calculator",
                                   run: function () { Sh.copy(fmt(v)) } })
        apps.forEach(function (a) { out.push({ kind: "Aplicación", title: a.name, sub: a.comment || "", icon: a.icon, run: function () { Apps.launch(a.id) } }) })
        settings.forEach(function (p) {
            if (p[1].toLowerCase().indexOf(ql) >= 0 || p[2].indexOf(ql) >= 0)
                out.push({ kind: "Ajustes", title: p[1], sub: "Ajustes de Lily", icon: "rros-settings", run: function () { Sh.detached(["rros-ajustes", "--pagina", p[0]]) } })
        })
        files.forEach(function (f) {
            out.push({ kind: "Archivo", title: f.split("/").pop(), sub: f.replace(Sh.home, "~"), icon: "text-x-generic",
                       run: function () { Sh.detached(["xdg-open", f]) } })
        })
        out.push({ kind: "Web", title: "Buscar «" + q + "» en la web", sub: "DuckDuckGo", icon: "internet-web-browser",
                   run: function () { Sh.detached(["xdg-open", "https://duckduckgo.com/?q=" + encodeURIComponent(q)]) } })
        return out
    }
}
