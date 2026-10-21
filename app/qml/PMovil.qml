// Móvil: el teléfono y el ordenador conectados con KDE Connect (avisos del móvil en Lily, enviar archivos, portapapeles
// compartido, hacer sonar el móvil). En el teléfono: la app «KDE Connect» (Android, F-Droid o iPhone).
import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import "."

RPage {
    id: p
    title: "Móvil"
    subtitle: "Tu teléfono y Lily conectados: avisos, archivos y portapapeles"
    icon: "qrc:/icons/sec-bluetooth.svg"
    property var devices: []      // [{ id, name, paired, reachable }]
    property bool have: true
    property string target: ""
    function refresh() {
        Sys.run(["kdeconnect-cli", "-l", "--id-name-only"], function (r) {
            p.have = r.code !== 127
            var all = r.out.split("\n").filter(function (l) { return l.trim() !== "" }).map(function (l) {
                var i = l.indexOf(" "); return { id: l.slice(0, i), name: l.slice(i + 1) }
            })
            Sys.run(["kdeconnect-cli", "-a", "--id-only"], function (r2) {
                var reach = r2.out.split("\n")
                Sys.run(["kdeconnect-cli", "--list-available", "--id-only"], function () {
                    p.devices = all.map(function (d) { d.reachable = reach.indexOf(d.id) >= 0; return d })
                })
            })
        })
        Sys.run(["kdeconnect-cli", "--refresh"], function () {})
    }
    function cmd(args, ok) { Sys.run(["kdeconnect-cli"].concat(args), function (r) { Ui.toast(r.code === 0 ? ok : (r.err || "No se pudo"), r.code !== 0); p.refresh() }) }
    onShown: refresh()
    Timer { interval: 5000; running: p.visible; repeat: true; onTriggered: p.refresh() }

    Note {
        visible: !p.have
        kind: "warn"
        text: "KDE Connect no está instalado. Instálalo desde la Tienda (KDE Connect)."
    }
    Card {
        SRow {
            title: "Cómo conectar"
            subtitle: "1. Instala «KDE Connect» en el móvil.  2. Conecta el móvil a la misma red Wi-Fi que este equipo.  3. Pulsa «Vincular» aquí y acepta en el teléfono."
            Btn { text: "Buscar"; onClicked: p.refresh() }
        }
    }
    Section { text: "Dispositivos" }
    Card {
        SRow { visible: p.devices.length === 0; title: "Ningún móvil a la vista"; subtitle: "Abre KDE Connect en el teléfono, en la misma red."; separator: false }
        Repeater {
            model: p.devices
            SRow {
                title: modelData.name
                subtitle: modelData.reachable ? "Conectado" : "Vinculado, fuera de alcance"
                separator: index < p.devices.length - 1
                RowLayout {
                    spacing: 6
                    Btn { text: "Vincular"; kind: "primary"; visible: !modelData.reachable; onClicked: p.cmd(["--pair", "-d", modelData.id], "Acepta en el teléfono") }
                    Btn { text: "Enviar archivo"; visible: modelData.reachable; onClicked: { p.target = modelData.id; file.open() } }
                    Btn { text: "Hacer sonar"; visible: modelData.reachable; onClicked: p.cmd(["--ring", "-d", modelData.id], "Sonando") }
                    Btn { text: "Desvincular"; kind: "danger"; visible: modelData.reachable; onClicked: p.cmd(["--unpair", "-d", modelData.id], "Desvinculado") }
                }
            }
        }
    }
    Card {
        SRow { title: "Avisos del móvil"; subtitle: "Llegan a Lily como cualquier aviso. Se ajustan en la app del teléfono." }
        SRow { title: "Portapapeles compartido"; subtitle: "Lo que copias en un sitio se puede pegar en el otro."; separator: false }
    }
    FileDialog { id: file; title: "Archivo para enviar al móvil"; onAccepted: p.cmd(["--share", decodeURIComponent(String(selectedFile)), "-d", p.target], "Enviado") }
}
