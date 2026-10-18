// Sonido: salida y entrada (dispositivo, volumen, balance, puerto y perfil), volumen por aplicación y sonidos del sistema.
// Con PipeWire y pactl (formato JSON).
import QtQuick
import QtQuick.Layouts
import "."

RPage {
    id: p
    title: "Sonido"
    subtitle: "Altavoces, auriculares, micrófono y el volumen de cada aplicación"
    icon: "qrc:/icons/sec-sonido.svg"
    property var sinks: []
    property var sources: []
    property var inputs: []        // aplicaciones que suenan
    property var cards: []
    property string defSink: ""
    property string defSource: ""

    function vol(o) {
        if (!o || !o.volume) return 0
        var ks = Object.keys(o.volume), t = 0
        ks.forEach(function (k) { t += parseInt(o.volume[k].value_percent) })
        return ks.length ? Math.round(t / ks.length) : 0
    }
    function bal(o) {
        if (!o || !o.volume || !o.volume["front-left"] || !o.volume["front-right"]) return 0
        var l = parseInt(o.volume["front-left"].value_percent), r = parseInt(o.volume["front-right"].value_percent)
        var m = Math.max(l, r)
        return m ? Math.round((r - l) / m * 100) : 0
    }
    function pj(args, cb) {
        Ui.run(["pactl", "-f", "json"].concat(args), function (r) { try { cb(JSON.parse(r.out)) } catch (e) { cb([]) } })
    }
    function refresh() {
        Ui.run(["pactl", "get-default-sink"], function (r) { defSink = r.out.trim() })
        Ui.run(["pactl", "get-default-source"], function (r) { defSource = r.out.trim() })
        pj(["list", "sinks"], function (l) { sinks = l })
        pj(["list", "sources"], function (l) { sources = l.filter(function (s) { return s.monitor_source === "" || s.name.indexOf(".monitor") < 0 }) })
        pj(["list", "sink-inputs"], function (l) { inputs = l })
        pj(["list", "cards"], function (l) { cards = l })
    }
    function pa(args) { Ui.run(["pactl"].concat(args), function (r) { if (r.code !== 0) Ui.toast(r.err.trim() || "Falló", true); refresh() }) }
    function setVol(kind, name, v, b) {
        var l = v, r = v
        if (b > 0) l = Math.round(v * (100 - b) / 100)
        if (b < 0) r = Math.round(v * (100 + b) / 100)
        pa(["set-" + kind + "-volume", name, l + "%", r + "%"])
    }
    function cardOf(o) {
        var id = o && o.properties ? o.properties["device.id"] : ""
        for (var i = 0; i < cards.length; i++) if (String(cards[i].properties["object.id"]) === String(id)) return cards[i]
        return null
    }
    function test() {
        var f = ["/usr/share/sounds/ocean/stereo/audio-volume-change.oga", "/usr/share/sounds/freedesktop/stereo/audio-volume-change.oga",
                 "/usr/share/sounds/ocean/stereo/bell.oga"]
        for (var i = 0; i < f.length; i++) if (Sys.exists(f[i])) { Sys.detached(["pw-play", f[i]]); return }
        Ui.toast("No hay sonidos de prueba instalados", true)
    }
    onShown: refresh()
    Timer { interval: 3000; running: p.visible; repeat: true; onTriggered: { p.pj(["list", "sink-inputs"], function (l) { p.inputs = l }) } }

    // ---- salida ----
    Section { text: "Salida" }
    Card {
        Repeater {
            model: p.sinks
            SRow {
                property bool def: modelData.name === p.defSink
                title: modelData.description
                subtitle: def ? "Se oye por aquí" : ""
                clickable: !def
                onClicked: p.pa(["set-default-sink", modelData.name])
                Rectangle { width: 18; height: 18; radius: 9; color: "transparent"; border.width: def ? 6 : 2; border.color: def ? Ui.accent : Ui.muted }
            }
        }
        SRow { visible: p.sinks.length === 0; title: "No hay salida de sonido"; subtitle: "¿Está en marcha PipeWire?"; separator: false }
    }
    Repeater {
        model: p.sinks.filter(function (s) { return s.name === p.defSink })
        Card {
            SRow {
                title: "Volumen"
                RowLayout {
                    spacing: 10
                    Btn { text: modelData.mute ? "🔇" : "🔊"; implicitWidth: 40; onClicked: p.pa(["set-sink-mute", modelData.name, "toggle"]) }
                    Slide { id: sv; from: 0; to: Ui.n("audio.max_volume", 100); value: p.vol(modelData); onCommitted: function (v) { p.setVol("sink", modelData.name, Math.round(v), p.bal(modelData)) } }
                    Txt { text: Math.round(sv.value) + " %"; color: Ui.muted; Layout.preferredWidth: 44 }
                }
            }
            SRow {
                title: "Balance"
                subtitle: "Izquierda ↔ derecha"
                RowLayout {
                    Slide { from: -100; to: 100; stepSize: 5; value: p.bal(modelData); onCommitted: function (v) { p.setVol("sink", modelData.name, p.vol(modelData), Math.round(v)) } }
                    Btn { text: "Centro"; onClicked: p.setVol("sink", modelData.name, p.vol(modelData), 0) }
                }
            }
            SRow {
                visible: modelData.ports && modelData.ports.length > 1
                title: "Conector"
                Combo {
                    options: (modelData.ports || []).map(function (x) { return [x.name, x.description + (x.availability === "not available" ? " (sin conectar)" : "")] })
                    value: modelData.active_port || ""
                    onPicked: function (v) { p.pa(["set-sink-port", modelData.name, v]) }
                }
            }
            SRow {
                id: cr
                property var card: p.cardOf(modelData)
                visible: card !== null && card.profiles && Object.keys(card.profiles).length > 1
                title: "Perfil de la tarjeta"
                subtitle: "Estéreo, envolvente, HDMI o, en auriculares Bluetooth, alta calidad o manos libres"
                separator: false
                Combo {
                    implicitWidth: 260
                    options: cr.card ? Object.keys(cr.card.profiles).filter(function (k) { return cr.card.profiles[k].available !== false })
                                                   .map(function (k) { return [k, cr.card.profiles[k].description] }) : []
                    value: cr.card ? cr.card.active_profile : ""
                    onPicked: function (v) { p.pa(["set-card-profile", cr.card.name, v]) }
                }
            }
        }
    }

    // ---- entrada ----
    Section { text: "Entrada" }
    Card {
        Repeater {
            model: p.sources
            SRow {
                property bool def: modelData.name === p.defSource
                title: modelData.description
                subtitle: def ? "Micrófono en uso" : ""
                clickable: !def
                onClicked: p.pa(["set-default-source", modelData.name])
                Rectangle { width: 18; height: 18; radius: 9; color: "transparent"; border.width: def ? 6 : 2; border.color: def ? Ui.accent : Ui.muted }
            }
        }
        Repeater {
            model: p.sources.filter(function (s) { return s.name === p.defSource })
            SRow {
                title: "Volumen del micrófono"
                separator: false
                RowLayout {
                    spacing: 10
                    Btn { text: modelData.mute ? "🎙✕" : "🎙"; implicitWidth: 44; onClicked: p.pa(["set-source-mute", modelData.name, "toggle"]) }
                    Slide { id: mv; from: 0; to: 150; value: p.vol(modelData); onCommitted: function (v) { p.pa(["set-source-volume", modelData.name, Math.round(v) + "%"]) } }
                    Txt { text: Math.round(mv.value) + " %"; color: Ui.muted; Layout.preferredWidth: 44 }
                }
            }
        }
        SRow { visible: p.sources.length === 0; title: "No hay micrófono"; separator: false }
    }

    // ---- aplicaciones ----
    Section { text: "Aplicaciones" }
    Card {
        Repeater {
            model: p.inputs
            SRow {
                title: modelData.properties["application.name"] || "Aplicación"
                subtitle: modelData.properties["media.name"] || ""
                RowLayout {
                    spacing: 8
                    Combo {
                        implicitWidth: 170
                        options: p.sinks.map(function (s) { return [String(s.index), s.description] })
                        value: String(modelData.sink)
                        onPicked: function (v) { p.pa(["move-sink-input", String(modelData.index), v]) }
                    }
                    Slide { implicitWidth: 140; from: 0; to: 150; value: p.vol(modelData); onCommitted: function (v) { p.pa(["set-sink-input-volume", String(modelData.index), Math.round(v) + "%"]) } }
                }
            }
        }
        SRow { visible: p.inputs.length === 0; title: "Ninguna aplicación está sonando ahora"; separator: false }
    }

    // ---- sistema ----
    Section { text: "Sonidos del sistema" }
    Card {
        KeyRow { k: "audio.event_sounds"; title: "Sonidos de avisos y eventos" }
        KeyRow { k: "audio.sound_theme"; title: "Tema de sonidos"; labels: { "ocean": "Ocean", "freedesktop": "Clásico" } }
        KeyRow { k: "audio.startup_sound"; title: "Sonido al iniciar sesión" }
        KeyRow { k: "audio.volume_feedback"; title: "Sonido al cambiar el volumen" }
        KeyRow { k: "audio.volume_step"; title: "Paso de las teclas de volumen"; unit: " %" }
        KeyRow { k: "audio.max_volume"; title: "Volumen máximo"; subtitle: "Por encima de 100 % amplifica (puede distorsionar)"; unit: " %"; step: 10 }
        SRow { title: "Probar los altavoces"; separator: false; Btn { text: "Reproducir"; onClicked: p.test() } }
    }
}
