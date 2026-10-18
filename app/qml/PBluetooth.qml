// Bluetooth: encender, buscar, emparejar, conectar y olvidar (bluetoothctl)
import QtQuick
import QtQuick.Layouts
import "."

RPage {
    id: p
    title: "Bluetooth"
    subtitle: "Auriculares, ratones, teclados y móviles"
    icon: "qrc:/icons/sec-bluetooth.svg"
    property bool service: true
    property bool powered: false
    property var paired: []
    property var near: []
    property bool scanning: false

    function parse(out) {
        return Ui.lines(out).filter(function (l) { return l.indexOf("Device ") === 0 }).map(function (l) {
            var f = l.split(" "); return { mac: f[1], name: f.slice(2).join(" ") }
        })
    }
    function refresh() {
        Ui.run(["systemctl", "is-active", "bluetooth"], function (r) {
            service = r.out.trim() === "active"
            if (!service) return
            Ui.run(["bluetoothctl", "show"], function (s) { powered = /Powered: yes/.test(s.out) })
            Ui.run(["bluetoothctl", "devices", "Paired"], function (d) {
                var list = parse(d.out)
                list.forEach(function (x) {
                    Ui.run(["bluetoothctl", "info", x.mac], function (i) {
                        x.connected = /Connected: yes/.test(i.out)
                        x.icon = (i.out.match(/Icon: (\S+)/) || [0, ""])[1]
                        x.battery = (i.out.match(/Battery Percentage: \S+ \((\d+)\)/) || [0, ""])[1]
                        paired = list.slice()
                    })
                })
                paired = list
            })
        })
    }
    function bt(args, msg) {
        Ui.run(["bluetoothctl"].concat(args), function (r) {
            if (r.code !== 0 || /Failed|not available/.test(r.out)) Ui.toast((r.out + r.err).trim().split("\n").pop() || "Falló", true)
            else if (msg) Ui.toast(msg, false)
            refresh()
        })
    }
    function scan() {
        scanning = true
        Ui.run(["bluetoothctl", "--timeout", "10", "scan", "on"], function () {
            Ui.run(["bluetoothctl", "devices"], function (r) {
                var pm = {}; paired.forEach(function (x) { pm[x.mac] = 1 })
                near = parse(r.out).filter(function (x) { return !pm[x.mac] && !/^([0-9A-F]{2}-){5}/.test(x.name) })
                scanning = false
            })
        })
    }
    function pair(d) {
        Ui.toast("Emparejando con " + d.name + "…", false)
        Ui.run(["bluetoothctl", "--timeout", "20", "pair", d.mac], function (r) {
            if (!/Pairing successful|AlreadyExists/.test(r.out)) { Ui.toast("No se pudo emparejar con " + d.name, true); return }
            Ui.run(["bluetoothctl", "trust", d.mac], function () { bt(["connect", d.mac], "Conectado a " + d.name) })
        })
    }
    onShown: refresh()

    Note {
        visible: !p.service
        kind: "warn"
        text: "El servicio de Bluetooth está apagado en esta edición (ahorra memoria). Actívalo para usarlo; pedirá la contraseña de administrador."
    }
    Card {
        visible: !p.service
        SRow { title: "Servicio de Bluetooth"; separator: false; Btn { text: "Activar"; kind: "primary"; onClicked: Ui.setKey("services.bluetooth", "true", function () { p.refresh() }) } }
    }
    Card {
        visible: p.service
        SRow {
            title: "Bluetooth"
            subtitle: p.powered ? "Encendido · visible para emparejar mientras esta página está abierta" : "Apagado"
            separator: false
            Toggle { checked: p.powered; onToggled: function (on) { p.bt(["power", on ? "on" : "off"]) } }
        }
    }
    Section { text: "Mis dispositivos"; visible: p.service && p.powered }
    Card {
        visible: p.service && p.powered
        Repeater {
            model: p.paired
            SRow {
                title: modelData.name
                subtitle: (modelData.connected ? "Conectado" : "No conectado") + (modelData.battery ? " · batería " + modelData.battery + " %" : "")
                RowLayout {
                    Btn { text: modelData.connected ? "Desconectar" : "Conectar"; onClicked: p.bt([modelData.connected ? "disconnect" : "connect", modelData.mac]) }
                    Btn { text: "Olvidar"; kind: "danger"; onClicked: p.bt(["remove", modelData.mac], "Olvidado") }
                }
            }
        }
        SRow { visible: p.paired.length === 0; title: "Ninguno todavía"; subtitle: "Pon el aparato en modo de emparejamiento y pulsa Buscar"; separator: false }
    }
    Section { text: "Cerca"; visible: p.service && p.powered }
    Card {
        visible: p.service && p.powered
        Repeater {
            model: p.near
            SRow { title: modelData.name; subtitle: modelData.mac; Btn { text: "Emparejar"; kind: "primary"; onClicked: p.pair(modelData) } }
        }
        SRow {
            title: p.scanning ? "Buscando durante 10 segundos…" : "Buscar aparatos"
            separator: false
            Btn { text: "Buscar"; busy: p.scanning; onClicked: p.scan() }
        }
    }
}
