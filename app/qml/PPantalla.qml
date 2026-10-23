// Pantallas: resolución, frecuencia, escala, giro y principal (kscreen-doctor), brillo y luz nocturna
import QtQuick
import QtQuick.Layouts
import "."

RPage {
    id: p
    title: "Pantallas"
    subtitle: "Resolución, escala, brillo y luz nocturna"
    icon: "qrc:/icons/sec-pantalla.svg"
    property var outputs: []
    property int bright: -1

    function refresh() {
        Ui.run(["kscreen-doctor", "-j"], function (r) {
            try { outputs = JSON.parse(r.out).outputs.filter(function (o) { return o.connected }) } catch (e) { outputs = [] }
        })
        Ui.run(["brightnessctl", "-m"], function (r) { var f = r.out.split(","); bright = r.code === 0 && f.length > 3 ? parseInt(f[3]) : -1 })
    }
    function kd(arg) {
        Ui.run(["kscreen-doctor", arg], function (r) { if (r.code !== 0) Ui.toast((r.err || "No se pudo cambiar la pantalla").trim(), true); refresh() })
    }
    function modeOf(o) { for (var i = 0; i < o.modes.length; i++) if (o.modes[i].id === o.currentModeId) return o.modes[i]; return null }
    function sizes(o) {
        var seen = {}, out = []
        o.modes.forEach(function (m) { var k = m.size.width + "x" + m.size.height; if (!seen[k]) { seen[k] = 1; out.push([k, m.size.width + " × " + m.size.height]) } })
        out.sort(function (a, b) { return parseInt(b[0]) - parseInt(a[0]) })
        return out
    }
    function rates(o) {
        var c = modeOf(o); if (!c) return []
        return o.modes.filter(function (m) { return m.size.width === c.size.width && m.size.height === c.size.height })
                      .map(function (m) { return [m.id, Math.round(m.refreshRate) + " Hz"] })
    }
    onShown: refresh()

    Repeater {
        model: p.outputs
        ColumnLayout {
            id: oc
            property var cur: p.modeOf(modelData)
            spacing: 10
            Layout.fillWidth: true
            Section { text: modelData.name + (p.outputs.length > 1 && modelData.priority === 1 ? " · principal" : "") }
            Card {
                SRow {
                    visible: p.outputs.length > 1
                    title: "Activa"
                    Toggle { checked: modelData.enabled; onToggled: function (on) { p.kd("output." + modelData.name + "." + (on ? "enable" : "disable")) } }
                }
                SRow {
                    title: "Resolución"
                    Combo {
                        options: p.sizes(modelData)
                        value: oc.cur ? oc.cur.size.width + "x" + oc.cur.size.height : ""
                        onPicked: function (v) {
                            // el modo de esa resolución con más hercios
                            var best = null
                            modelData.modes.forEach(function (m) { if (m.size.width + "x" + m.size.height === v && (!best || m.refreshRate > best.refreshRate)) best = m })
                            if (best) p.kd("output." + modelData.name + ".mode." + best.id)
                        }
                    }
                }
                SRow {
                    title: "Frecuencia"
                    Combo {
                        options: p.rates(modelData)
                        value: modelData.currentModeId
                        onPicked: function (v) { p.kd("output." + modelData.name + ".mode." + v) }
                    }
                }
                SRow {
                    title: "Escala"
                    subtitle: "Todo más grande en pantallas de mucha resolución"
                    Seg {
                        options: [["1", "100 %"], ["1.25", "125 %"], ["1.5", "150 %"], ["1.75", "175 %"], ["2", "200 %"]]
                        value: String(modelData.scale)
                        onPicked: function (v) { p.kd("output." + modelData.name + ".scale." + v) }
                    }
                }
                SRow {
                    title: "Giro"
                    Seg {
                        options: [["1", "Normal"], ["2", "90°"], ["4", "180°"], ["8", "270°"]]
                        value: String(modelData.rotation)
                        onPicked: function (v) { p.kd("output." + modelData.name + ".rotation." + ({ "1": "normal", "2": "left", "4": "inverted", "8": "right" })[v]) }
                    }
                }
                SRow {
                    visible: p.outputs.length > 1
                    title: "Pantalla principal"
                    subtitle: "Donde están la barra y el dock"
                    separator: false
                    Btn { text: modelData.priority === 1 ? "Es la principal" : "Hacer principal"; enabled2: modelData.priority !== 1; onClicked: p.kd("output." + modelData.name + ".priority.1") }
                }
            }
        }
    }
    Note { visible: p.outputs.length === 0; text: "No se pudieron leer las pantallas (hace falta una sesión de Plasma)." }

    Section { text: "Brillo"; visible: p.bright >= 0 }
    Card {
        visible: p.bright >= 0
        SRow {
            title: "Brillo de la pantalla"
            separator: false
            Slide { from: 1; to: 100; value: Math.max(1, p.bright); onCommitted: function (v) { Ui.run(["brightnessctl", "set", Math.round(v) + "%"], function () {}) } }
        }
    }
    Section { text: "Fluidez" }
    Card {
        KeyRow { k: "display.max_refresh"; title: "Frecuencia más alta"; separator: false
                 subtitle: "Al iniciar sesión, cada pantalla a su máximo: 120 Hz o más si la admite (en máquinas virtuales suele ser 60 Hz)"
                 onValueChanged: if (value === "true") Sys.detached(["/usr/libexec/rros-max-hz"]) }
    }

    Section { text: "Luz nocturna" }
    Card {
        KeyRow { k: "ui.night_light"; title: "Luz nocturna"; subtitle: "Colores más cálidos para descansar la vista"; labels: { "off": "Apagada", "auto": "Al anochecer", "on": "Siempre" } }
        KeyRow { k: "ui.night_light_temp"; title: "Temperatura"; unit: " K"; step: 100; separator: false }
    }
}
