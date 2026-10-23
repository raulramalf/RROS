// Actualizaciones: las que hay pendientes y actualizar todo con su progreso
import QtQuick
import QtQuick.Layouts
import "."

RPage {
    id: p
    title: "Actualizaciones"
    subtitle: "Lily descarga las novedades cada día; tú decides cuándo instalarlas"
    icon: "qrc:/icons/app-actualizar.svg"
    property var pending: []
    property bool checking: false
    property bool working: false
    property string log: ""

    function check() {
        checking = true
        Ui.run(["checkupdates", "--nocolor"], function (r) {
            checking = false
            pending = Ui.lines(r.out).map(function (l) { var f = l.split(" "); return { name: f[0], from: f[1], to: f[3] } })
        })
    }
    function update() {
        working = true; log = "Preparando…"
        // sin las líneas de pacman: la fase y el porcentaje («( 3/40) …» de pacman)
        Sys.stream(["pkexec", "rros-apps", "update"], function (l) {
            var m = /\(\s*(\d+)\s*\/\s*(\d+)\s*\)/.exec(l)
            var ph = /descarg|download|retriev/i.test(l) ? "Descargando" : /instal|upgrad|actualiz/i.test(l) ? "Instalando" : ""
            if (ph !== "") p.log = ph + "…" + (m ? "  " + Math.round(100 * Number(m[1]) / Number(m[2])) + " %" : "")
        }, function (code) {
            p.working = false
            Ui.toast(code === 0 ? "Sistema al día" : "La actualización no terminó", code !== 0)
            p.check()
        })
    }
    onShown: if (!checking && pending.length === 0) check()

    Card {
        SRow {
            title: p.checking ? "Buscando…" : p.pending.length === 0 ? "Lily está al día" : p.pending.length + " actualizaciones"
            subtitle: p.working ? p.log : (p.pending.length ? "Se recomienda reiniciar después si se actualiza el kernel" : "")
            separator: false
            RowLayout {
                Btn { text: "Buscar"; busy: p.checking; onClicked: p.check() }
                Btn { text: "Actualizar todo"; kind: "primary"; visible: p.pending.length > 0; busy: p.working; onClicked: p.update() }
            }
        }
    }
    Card {
        visible: p.pending.length > 0
        Repeater {
            model: p.pending.slice(0, 200)
            SRow { title: modelData.name; subtitle: modelData.from + " → " + modelData.to; separator: index < Math.min(p.pending.length, 200) - 1 }
        }
    }
    Section { text: "Automático" }
    Card {
        KeyRow { k: "system.auto_updates"; title: "Descargar las actualizaciones cada día"; subtitle: "Se instalan solo cuando tú lo pides: en Arch, actualizar sin mirar puede romper algo"; separator: false }
    }
}
