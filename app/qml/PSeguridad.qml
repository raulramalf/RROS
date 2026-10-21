// Seguridad y privacidad: antivirus (análisis, tiempo real, cuarentena), directivas, auditoría, nivel, cortafuegos y
// privacidad. El antivirus es rros-antivirus sobre ClamAV; los análisis de tus carpetas van sin contraseña y los del
// sistema entero, con la de administrador.
import QtQuick
import QtQuick.Dialogs
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
    // ---- antivirus ----
    property var av: null
    property var quarantine: []
    property bool scanning: false
    property int scanTotal: 0
    property int scanDone: 0
    property var threats: []
    property string scanResult: ""
    function avRefresh() {
        Sys.run(["rros-antivirus", "estado", "--json"], function (r) { try { p.av = JSON.parse(r.out) } catch (e) { p.av = null } })
        Sys.run(["rros-antivirus", "cuarentena", "--json"], function (r) { try { p.quarantine = JSON.parse(r.out) } catch (e) { p.quarantine = [] } })
    }
    function scan(mode) {
        scanning = true; scanTotal = 0; scanDone = 0; threats = []; scanResult = ""
        var argv = mode === "completo" ? ["pkexec", "rros-antivirus", "analizar", "completo"] : ["rros-antivirus", "analizar", mode]
        Sys.stream(argv, function (l) {
            if (l.indexOf("TOTAL ") === 0) p.scanTotal = parseInt(l.slice(6))
            else if (l.indexOf("PROGRESO ") === 0) p.scanDone = parseInt(l.slice(9))
            else if (l.indexOf("AMENAZA ") === 0) p.threats = p.threats.concat([l.slice(8).split("|")])
            else if (l.indexOf("RESULTADO ") === 0) {
                try {
                    var r = JSON.parse(l.slice(10))
                    p.scanResult = r.error ? r.error
                        : r.amenazas === 0 ? "Sin amenazas: " + r.archivos + " archivos analizados en " + r.segundos + " s."
                        : r.amenazas + (r.amenazas === 1 ? " amenaza" : " amenazas") + " (" + r.en_cuarentena + " en cuarentena) en " + r.archivos + " archivos."
                } catch (e) {}
            }
        }, function (code) { p.scanning = false; if (p.scanResult === "" && code !== 0) p.scanResult = "No se pudo analizar"; p.avRefresh() })
    }
    function qAction(what, id) {
        Sys.run(["rros-antivirus", what, id], function (r) {
            Ui.toast(r.code === 0 ? (what === "restaurar" ? "Archivo restaurado" : "Borrado para siempre") : (r.err || "No se pudo"), r.code !== 0)
            p.avRefresh()
        })
    }
    onShown: { refresh(); avRefresh() }

    Section { text: "Protección antivirus" }
    Card {
        SRow {
            id: avRow
            readonly property bool good: p.av && p.av.firmas !== "" && (p.av.tiempo_real || p.av.programado)
            title: !p.av || !p.av.motor ? "Antivirus no instalado"
                 : p.av.firmas === "" ? "Sin firmas de virus"
                 : p.av.tiempo_real ? "Protegido en tiempo real" : p.av.programado ? "Protegido con análisis programados" : "Protección desactivada"
            subtitle: !p.av ? "" : (p.av.firmas === "" ? "Descarga las firmas para poder analizar (hace falta Internet)."
                     : "Firmas " + p.av.firmas + (p.av.firmas_fecha ? " · " + p.av.firmas_fecha : "") + (p.av.ultimo ? " · último análisis: " + p.av.ultimo.fecha.slice(0, 16).replace("T", " ") + ", " + p.av.ultimo.amenazas + " amenazas" : " · todavía sin análisis"))
            RowLayout {
                spacing: 10
                Rectangle { width: 12; height: 12; radius: 6; color: avRow.good ? Ui.ok : Ui.warn }
                Btn { text: "Actualizar firmas"; onClicked: Sys.run(["pkexec", "rros-antivirus", "actualizar"], function (r) { Ui.toast(r.code === 0 ? "Firmas al día" : "No se pudieron descargar", r.code !== 0); p.avRefresh() }) }
            }
        }
        SRow {
            title: "Analizar"
            subtitle: "Rápido: Descargas, Escritorio, Documentos y /tmp. Completo: todo el equipo (pide la contraseña). O elige una carpeta."
            RowLayout {
                spacing: 8
                Btn { text: "Rápido"; kind: "primary"; enabled: !p.scanning; onClicked: p.scan("rapido") }
                Btn { text: "Completo"; enabled: !p.scanning; onClicked: p.scan("completo") }
                Btn { text: "Carpeta…"; enabled: !p.scanning; onClicked: folderPick.open() }
            }
        }
        SRow {
            visible: p.scanning || p.scanResult !== ""
            title: p.scanning ? "Analizando… " + (p.scanTotal ? Math.min(100, Math.round(100 * p.scanDone / p.scanTotal)) + " %" : "") : p.scanResult
            subtitle: p.threats.length ? p.threats.map(function (t) { return t[0] + " — " + t[1] }).join("\n") : ""
            Rectangle {
                visible: p.scanning
                width: 220; height: 8; radius: 4; color: Ui.track
                Rectangle { width: parent.width * (p.scanTotal ? Math.min(1, p.scanDone / p.scanTotal) : 0.05); height: 8; radius: 4; color: Ui.accent }
            }
        }
        KeyRow { k: "security.av_realtime"; title: "Protección en tiempo real"; subtitle: "Analiza cada archivo de tu carpeta personal al abrirlo o guardarlo. Usa cerca de 1 GB de RAM." }
        KeyRow { k: "security.av_schedule"; title: "Análisis completo programado"; labels: { "off": "Nunca", "diario": "Cada día", "semanal": "Cada semana" } }
        KeyRow { k: "security.av_action"; title: "Si encuentra una amenaza"; labels: { "cuarentena": "Cuarentena", "avisar": "Solo avisar" } }
        KeyRow { k: "security.av_updates"; title: "Actualizar las firmas solas" }
        KeyRow { k: "security.av_max_file_mb"; title: "Tamaño máximo de archivo analizado"; unit: " MB"; step: 50; separator: false }
    }
    Section { text: "Cuarentena"; visible: p.quarantine.length > 0 }
    Card {
        visible: p.quarantine.length > 0
        Repeater {
            model: p.quarantine
            SRow {
                title: modelData.ruta
                subtitle: modelData.amenaza + " · " + modelData.fecha.slice(0, 16).replace("T", " ")
                separator: index < p.quarantine.length - 1
                RowLayout {
                    Btn { text: "Restaurar"; onClicked: p.qAction("restaurar", modelData.id) }
                    Btn { text: "Borrar"; kind: "danger"; onClicked: p.qAction("borrar", modelData.id) }
                }
            }
        }
    }
    FolderDialog { id: folderPick; title: "Carpeta que quieres analizar"; onAccepted: p.scan(decodeURIComponent(String(selectedFolder).replace("file://", ""))) }

    Section { text: "Directivas de seguridad" }
    Card {
        KeyRow { k: "security.password_min_length"; title: "Longitud mínima de las contraseñas"; subtitle: "Para las contraseñas nuevas. 0: sin mínimo"; unit: "" }
        KeyRow { k: "security.password_complex"; title: "Contraseñas complejas"; subtitle: "Mayúsculas, minúsculas, números y símbolos" }
        KeyRow { k: "security.login_attempts"; title: "Bloquear la cuenta tras contraseñas fallidas"; subtitle: "Durante 10 minutos. 0: nunca" }
        KeyRow { k: "security.sudo_timeout_minutes"; title: "Recordar la contraseña de administrador"; unit: " min" }
        KeyRow { k: "security.usb_guard"; title: "Bloquear los dispositivos USB nuevos"; subtitle: "Los conectados al activarlo quedan permitidos; los nuevos hay que autorizarlos" }
        KeyRow { k: "security.usb_storage"; title: "Memorias y discos USB" }
        KeyRow { k: "security.camera"; title: "Cámara" }
        KeyRow { k: "security.tmp_noexec"; title: "No ejecutar programas desde /tmp"; subtitle: "Frena muchos ataques que descargan y ejecutan algo ahí. Se aplica al reiniciar." }
        KeyRow { k: "security.kernel_hardening"; title: "Kernel endurecido" }
        KeyRow { k: "security.core_dumps"; title: "Guardar volcados de memoria de los programas que fallan"; separator: false }
    }

    Section { text: "Estado" }
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
        KeyRow { k: "privacy.telemetry"; title: "Enviar datos de uso"; subtitle: "Lily no envía nada a nadie" }
        KeyRow { k: "privacy.crash_reports"; title: "Informes de fallos" }
        KeyRow { k: "services.location"; title: "Ubicación"; separator: false }
    }
}
