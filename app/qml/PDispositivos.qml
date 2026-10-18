// Dispositivos: discos y memorias (montar, expulsar), USB, radios y el resto del hardware
import QtQuick
import QtQuick.Layouts
import "."

RPage {
    id: p
    title: "Dispositivos"
    subtitle: "Discos, memorias USB, radios y el resto del hardware"
    icon: "qrc:/icons/app-dispositivos.svg"
    property var dev: null
    function refresh() { Ui.api("GET", "/api/devices", {}, function (r) { dev = r && r.devices ? r : null }) }
    function act(a, id) { Ui.api("POST", "/api/device_action", { action: a, id: id }, function (r) { Ui.toast(r.ok ? "Hecho" : (r.output || r.error || "Falló").trim(), !r.ok); refresh() }) }
    onShown: refresh()
    Timer { interval: 4000; running: p.visible; repeat: true; onTriggered: p.refresh() }

    Repeater {
        model: p.dev ? p.dev.categories.filter(function (c) { return p.dev.devices.some(function (d) { return d.cat === c.id }) }) : []
        ColumnLayout {
            property string cat: modelData.id
            spacing: 10
            Layout.fillWidth: true
            Section { text: modelData.label }
            Card {
                Repeater {
                    model: p.dev.devices.filter(function (d) { return d.cat === cat })
                    SRow {
                        id: dr
                        title: modelData.name
                        subtitle: [modelData.detail, modelData.state].filter(function (x) { return x }).join(" · ")
                        RowLayout {
                            Repeater {
                                model: modelData.actions ? modelData.actions.split(",") : []
                                Btn {
                                    text: ({ mount: "Abrir", unmount: "Desmontar", eject: "Expulsar", block: "Apagar", unblock: "Encender" })[modelData] || modelData
                                    onClicked: p.act(modelData, dr.devId)
                                }
                            }
                        }
                        property string devId: modelData.id
                    }
                }
            }
        }
    }
}
