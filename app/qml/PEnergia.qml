// Batería y energía: estado de la batería, modo de energía, apagado de pantalla, suspensión y tapa
import QtQuick
import QtQuick.Layouts
import "."

RPage {
    id: p
    title: "Batería y energía"
    subtitle: "Rendimiento o duración de la batería"
    icon: "qrc:/icons/sec-energia.svg"
    property var bat: null
    property string profile: ""
    property bool ppd: Sys.hasProgram("powerprofilesctl")

    function refresh() {
        Ui.run(["sh", "-c", "for b in /sys/class/power_supply/BAT*; do [ -d \"$b\" ] && cat $b/capacity $b/status $b/energy_full $b/energy_full_design 2>/dev/null; break; done"], function (r) {
            var l = Ui.lines(r.out)
            bat = l.length >= 2 ? { pct: parseInt(l[0]), status: l[1], health: l.length >= 4 ? Math.round(100 * parseInt(l[2]) / parseInt(l[3])) : 0 } : null
        })
        if (ppd) Ui.run(["powerprofilesctl", "get"], function (r) { profile = r.out.trim() })
    }
    onShown: refresh()

    Card {
        visible: p.bat !== null
        SRow {
            title: p.bat ? "Batería al " + p.bat.pct + " %" : ""
            subtitle: p.bat ? ({ "Charging": "Cargando", "Discharging": "Con batería", "Full": "Cargada", "Not charging": "Enchufado, sin cargar" }[p.bat.status] || p.bat.status) +
                              (p.bat.health ? " · salud " + p.bat.health + " %" : "") : ""
            separator: false
            Rectangle {
                width: 120; height: 14; radius: 7; color: Ui.track
                Rectangle { width: parent.width * (p.bat ? p.bat.pct : 0) / 100; height: parent.height; radius: 7; color: p.bat && p.bat.pct <= 15 ? Ui.danger : Ui.ok }
            }
        }
    }
    Card {
        visible: p.ppd
        SRow {
            title: "Modo de energía"
            subtitle: "Ahorro alarga la batería; Rendimiento da toda la potencia"
            separator: false
            Seg {
                options: [["power-saver", "Ahorro"], ["balanced", "Equilibrado"], ["performance", "Rendimiento"]]
                value: p.profile
                onPicked: function (v) { Ui.run(["powerprofilesctl", "set", v], function (r) { if (r.code !== 0) Ui.toast("Este equipo no tiene ese modo", true); p.refresh() }) }
            }
        }
    }
    Section { text: "Inactividad" }
    Card {
        KeyRow { k: "power.screen_off_minutes"; title: "Apagar la pantalla tras"; subtitle: "0: nunca"; unit: " min" }
        KeyRow { k: "power.suspend_minutes"; title: "Suspender tras"; subtitle: "0: nunca. Con batería, antes."; unit: " min" }
        KeyRow { k: "privacy.lock_minutes"; title: "Bloquear la sesión tras"; subtitle: "0: no bloquear"; unit: " min" }
        KeyRow { k: "power.lid_action"; title: "Al cerrar la tapa"; labels: { "ignore": "Nada", "suspend": "Suspender", "hibernate": "Hibernar", "poweroff": "Apagar" } }
        KeyRow { k: "power.battery_low_percent"; title: "Aviso de batería baja"; unit: " %"; separator: false }
    }
}
