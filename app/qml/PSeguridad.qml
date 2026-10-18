// Seguridad y privacidad: auditoría, nivel, cortafuegos y privacidad
import QtQuick
import QtQuick.Layouts
import "."

RPage {
    id: p
    title: "Seguridad y privacidad"
    subtitle: "Cómo está protegido el equipo"
    icon: "qrc:/icons/app-seguridad.svg"
    property var sec: null
    property bool busy: false
    function refresh() { Ui.api("GET", "/api/security", {}, function (r) { sec = r && r.checks ? r : null }) }
    function level(l) {
        busy = true
        Ui.api("POST", "/api/security_level", { level: l }, function (r) { busy = false; Ui.toast(r.ok ? "Nivel aplicado" : r.error, !r.ok); refresh(); Ui.reload() })
    }
    onShown: refresh()

    Card {
        SRow {
            title: p.sec ? "Puntuación: " + p.sec.score + " de 100" : "Comprobando…"
            subtitle: p.sec ? "Nivel " + p.sec.level : ""
            separator: false
            Rectangle {
                width: 160; height: 10; radius: 5; color: Ui.track
                Rectangle { width: parent.width * (p.sec ? p.sec.score : 0) / 100; height: 10; radius: 5; color: p.sec && p.sec.score >= 70 ? Ui.ok : p.sec && p.sec.score >= 40 ? Ui.warn : Ui.danger }
            }
        }
    }
    Section { text: "Nivel de seguridad" }
    Card {
        SRow {
            title: "Nivel"
            subtitle: "Básico: cortafuegos. Estándar (recomendado): además kernel endurecido y bloqueo tras 10 intentos. Estricto: sin USB ni cámara, sin ping ni SSH."
            separator: false
            Seg { options: [["basico", "Básico"], ["estandar", "Estándar"], ["estricto", "Estricto"]]; value: p.sec ? p.sec.level : ""; onPicked: function (v) { p.level(v) } }
        }
    }
    Section { text: "Comprobaciones" }
    Card {
        Repeater {
            model: p.sec ? p.sec.checks : []
            SRow {
                title: modelData.title
                subtitle: modelData.detail
                RowLayout {
                    Rectangle { width: 10; height: 10; radius: 5; color: modelData.status === "ok" ? Ui.ok : modelData.status === "warn" ? Ui.warn : modelData.status === "bad" ? Ui.danger : Ui.muted }
                }
            }
        }
    }
    Section { text: "Cortafuegos" }
    Card {
        KeyRow { k: "security.firewall"; title: "Cortafuegos"; subtitle: "Bloquea las conexiones que no has pedido" }
        KeyRow { k: "security.firewall_ping"; title: "Responder al ping" }
        KeyRow { k: "security.firewall_allow"; title: "Puertos abiertos"; subtitle: "Separados por comas, p. ej. 22/tcp, 8080/tcp"; separator: false }
    }
    Section { text: "Privacidad" }
    Card {
        KeyRow { k: "privacy.telemetry"; title: "Enviar datos de uso"; subtitle: "RROS no envía nada a nadie" }
        KeyRow { k: "privacy.crash_reports"; title: "Informes de fallos" }
        KeyRow { k: "security.usb_storage"; title: "Memorias USB" }
        KeyRow { k: "security.camera"; title: "Cámara" }
        KeyRow { k: "services.location"; title: "Ubicación" }
        KeyRow { k: "security.login_attempts"; title: "Bloquear tras contraseñas fallidas"; subtitle: "0: nunca" }
        KeyRow { k: "security.sudo_timeout_minutes"; title: "Recordar la contraseña de administrador"; unit: " min"; separator: false }
    }
}
