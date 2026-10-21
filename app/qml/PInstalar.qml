// Instalar Lily (solo en el sistema en vivo): idioma, disco, usuario, resumen y progreso
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

RPage {
    id: p
    title: "Instalar Lily"
    subtitle: "Cuatro pasos y Lily queda en el disco. Puede ir junto a Windows (en el espacio libre) y cifrado."
    icon: "qrc:/icons/app-instalar.svg"
    property int step: 0
    property var disks: null
    property var status: null
    property var f: ({ locale: "es_ES.UTF-8", keymap: "es", timezone: "Europe/Madrid", disk: "", mode: "", encrypt: false,
                       passphrase: "", passphrase2: "", fullname: "", user: "", password: "", password2: "", hostname: "rros", confirm: false })
    readonly property var steps: ["Idioma", "Disco", "Usuario", "Resumen"]

    function setF(k, v) { var x = f; x[k] = v; f = x }
    function diskOf() { if (!disks) return null; for (var i = 0; i < disks.disks.length; i++) if (disks.disks[i].path === f.disk) return disks.disks[i]; return null }
    function slug(s) { return s.toLowerCase().normalize("NFD").replace(/[̀-ͯ]/g, "").replace(/[^a-z0-9]/g, "").slice(0, 30) }
    function loadDisks() { disks = null; Ui.api("GET", "/api/install_disks", {}, function (r) { disks = r && r.disks ? r : { disks: [], error: (r && r.error) || "No se pudieron leer los discos" } }) }
    function poll() { Ui.api("GET", "/api/install_status", {}, function (r) { status = r }) }
    function problem() {
        if (step === 1) {
            if (!f.disk || !f.mode) return "Elige un disco y cómo instalar"
            if (f.encrypt && f.passphrase.length < 8) return "La contraseña del cifrado necesita 8 caracteres o más"
            if (f.encrypt && f.passphrase !== f.passphrase2) return "Las contraseñas del cifrado no coinciden"
        }
        if (step === 2) {
            if (!/^[a-z_][a-z0-9_-]{0,31}$/.test(f.user)) return "El nombre de usuario: minúsculas y números, sin espacios"
            if (f.password.length < 4) return "Elige una contraseña"
            if (f.password !== f.password2) return "Las contraseñas no coinciden"
            if (!/^[a-zA-Z0-9-]{1,63}$/.test(f.hostname)) return "El nombre del equipo: letras, números y guiones"
        }
        if (step === 3 && f.mode === "wipe" && !f.confirm) return "Confirma que se borrará el disco"
        return ""
    }
    function next() {
        var e = problem()
        if (e) { Ui.toast(e, true); return }
        if (step < 3) { step++; if (step === 1 && !disks) loadDisks(); return }
        Ui.api("POST", "/api/install", { disk: f.disk, mode: f.mode, encrypt: f.encrypt ? "1" : "0", passphrase: f.encrypt ? f.passphrase : "",
                                          user: f.user, fullname: f.fullname, password: f.password, hostname: f.hostname,
                                          keymap: f.keymap, locale: f.locale, timezone: f.timezone }, function (r) {
            if (!r.ok) { Ui.toast(r.error, true); return }
            setF("password", ""); setF("password2", ""); setF("passphrase", ""); setF("passphrase2", "")
            poll()
        })
    }
    onShown: poll()
    Timer { interval: 1500; repeat: true; running: p.visible && p.status && p.status.started && !p.status.done; onTriggered: p.poll() }

    Note { visible: !Sys.live; text: "Lily ya está instalado en este equipo. El instalador solo está en el sistema en vivo (la ISO o la memoria USB)." }

    // pasos
    RowLayout {
        visible: Sys.live && !(p.status && p.status.started)
        spacing: 8
        Repeater {
            model: p.steps
            Rectangle {
                implicitWidth: sl.implicitWidth + 26; implicitHeight: 28; radius: 14
                color: index === p.step ? Ui.accent : index < p.step ? Qt.rgba(Qt.color(Ui.accent).r, Qt.color(Ui.accent).g, Qt.color(Ui.accent).b, 0.25) : Ui.field
                Txt { id: sl; anchors.centerIn: parent; text: (index + 1) + ". " + modelData; color: index === p.step ? Ui.accentText : Ui.textColor; font.pixelSize: Ui.fs - 1 }
            }
        }
    }

    // 1. idioma
    Card {
        visible: Sys.live && p.step === 0 && !(p.status && p.status.started)
        SRow {
            title: "Idioma"
            Combo {
                options: [["es_ES.UTF-8", "Español (España)"], ["es_MX.UTF-8", "Español (México)"], ["es_AR.UTF-8", "Español (Argentina)"], ["ca_ES.UTF-8", "Català"], ["en_US.UTF-8", "English"]]
                value: p.f.locale
                onPicked: function (v) { p.setF("locale", v); p.setF("timezone", { "es_MX.UTF-8": "America/Mexico_City", "es_AR.UTF-8": "America/Argentina/Buenos_Aires", "en_US.UTF-8": "America/New_York" }[v] || "Europe/Madrid") }
            }
        }
        SRow {
            title: "Teclado"
            Combo { options: [["es", "Español"], ["latam", "Latinoamericano"], ["us", "Inglés (EE. UU.)"], ["gb", "Inglés (Reino Unido)"]]; value: p.f.keymap; onPicked: function (v) { p.setF("keymap", v) } }
        }
        SRow { title: "Zona horaria"; Field { text: p.f.timezone; onEditingFinished: p.setF("timezone", text) } }
        SRow { title: "Prueba el teclado"; separator: false; Field { placeholderText: "ñ ç ¿? @ € …" } }
    }

    // 2. disco
    ColumnLayout {
        visible: Sys.live && p.step === 1 && !(p.status && p.status.started)
        spacing: 12
        Layout.fillWidth: true
        Note { visible: p.disks === null; text: "Buscando discos…" }
        Note { visible: p.disks !== null && !!p.disks.error; text: p.disks ? p.disks.error || "" : ""; kind: "error" }
        Txt { visible: p.disks !== null; color: Ui.muted; text: p.disks ? "Arranque " + (p.disks.firmware === "uefi" ? "UEFI" : "BIOS") + " · Lily " + p.disks.profile + " necesita " + Math.round(p.disks.min_mb / 1024) + " GB" : "" }
        Repeater {
            model: p.disks ? p.disks.disks : []
            Card {
                id: dc
                property string diskPath: modelData.path
                property real modelData_free: modelData.free_mb || 0
                // el disco dibujado: cada partición a su escala y, en verde, dónde iría Lily
                Item {
                    Layout.fillWidth: true
                    implicitHeight: 64
                    readonly property real total: Math.max(1, modelData.size / 1048576)
                    Row {
                        id: bar
                        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 14 }
                        height: 26
                        spacing: 2
                        Repeater {
                            model: modelData.parts
                            Rectangle {
                                width: Math.max(4, (bar.width - 2 * modelData.parts.length) * modelData.size_mb / parent.parent.parent.total)
                                height: 26; radius: 5
                                color: modelData.role === "windows" ? "#3b82f6" : modelData.role === "esp" ? "#8a8f98" : modelData.role === "linux" ? "#f59e0b" : "#6b7280"
                                Txt { anchors.centerIn: parent; visible: parent.width > 60; text: modelData.label || modelData.fstype || modelData.name; color: "white"; font.pixelSize: 10; elide: Text.ElideRight; width: parent.width - 8; horizontalAlignment: Text.AlignHCenter }
                            }
                        }
                        Rectangle {
                            visible: dc.modelData_free > 0
                            width: Math.max(6, bar.width * dc.modelData_free / parent.parent.total); height: 26; radius: 5
                            color: p.f.disk === dc.diskPath && p.f.mode === "alongside" ? Ui.ok : "transparent"
                            border.color: Ui.line; border.width: 1
                            Txt { anchors.centerIn: parent; visible: parent.width > 60; text: p.f.disk === dc.diskPath && p.f.mode === "alongside" ? "Lily" : "libre"; font.pixelSize: 10 }
                        }
                    }
                    Rectangle {   // con «borrar todo», el disco entero en verde
                        visible: p.f.disk === dc.diskPath && p.f.mode === "wipe"
                        anchors.fill: bar; radius: 5; color: Ui.ok
                        Txt { anchors.centerIn: parent; text: "Lily (todo el disco)"; color: "white"; font.pixelSize: 11; font.weight: Font.DemiBold }
                    }
                    Row {
                        anchors { left: parent.left; top: bar.bottom; topMargin: 6; leftMargin: 14 }
                        spacing: 14
                        Repeater {
                            model: [["#3b82f6", "Windows"], ["#8a8f98", "Arranque (EFI)"], ["#f59e0b", "Linux"], ["#6b7280", "Otros"], [Ui.ok, "Lily"]]
                            Row { spacing: 4; Rectangle { width: 10; height: 10; radius: 3; color: modelData[0]; anchors.verticalCenter: parent.verticalCenter }
                                  Txt { text: modelData[1]; font.pixelSize: 10; color: Ui.muted } }
                        }
                    }
                }
                SRow {
                    title: modelData.model + " · " + modelData.size_h
                    subtitle: modelData.path + (modelData.systems ? " · tiene " + modelData.systems : "") + (modelData.free_h ? " · libre " + modelData.free_h : "") + (modelData.in_use ? "\nNo disponible: " + modelData.in_use : "")
                }
                Repeater {
                    model: modelData.in_use ? [] : [["alongside", modelData.systems ? "Junto a " + modelData.systems + " (en el espacio libre)" : "En el espacio libre, sin borrar nada", modelData.alongside],
                                                    ["wipe", "Borrar todo el disco e instalar Lily", modelData.wipe]]
                    SRow {
                        property bool on: p.f.disk === dc.diskPath && p.f.mode === modelData[0]
                        title: modelData[1]
                        subtitle: modelData[2] ? "No se puede: " + modelData[2] : ""
                        enabled2: !modelData[2]
                        clickable: !modelData[2]
                        separator: index === 0
                        onClicked: { p.setF("disk", dc.diskPath); p.setF("mode", modelData[0]); p.setF("confirm", false) }
                        Rectangle { width: 18; height: 18; radius: 9; color: "transparent"; border.width: on ? 6 : 2; border.color: on ? Ui.accent : Ui.muted }
                    }
                }
            }
        }
        Card {
            SRow {
                title: "Cifrar el disco"
                subtitle: "Al encender se pide una contraseña antes de arrancar (LUKS2). Si la olvidas, los datos no se pueden recuperar."
                Toggle { checked: p.f.encrypt; onToggled: function (on) { p.setF("encrypt", on) } }
            }
            SRow { visible: p.f.encrypt; title: "Contraseña del cifrado"; Field { echoMode: TextInput.Password; onTextChanged: p.setF("passphrase", text) } }
            SRow { visible: p.f.encrypt; title: "Repítela"; separator: false; Field { echoMode: TextInput.Password; onTextChanged: p.setF("passphrase2", text) } }
        }
    }

    // 3. usuario
    Card {
        visible: Sys.live && p.step === 2 && !(p.status && p.status.started)
        SRow { title: "Tu nombre"; Field { text: p.f.fullname; onTextChanged: { p.setF("fullname", text); p.setF("user", p.slug(text)); p.setF("hostname", (p.slug(text) || "mi") + "-rros") } } }
        SRow { title: "Nombre de usuario"; subtitle: "Minúsculas, sin espacios"; Field { text: p.f.user; onEditingFinished: p.setF("user", text) } }
        SRow { title: "Contraseña"; subtitle: "Para entrar y para tareas de administración"; Field { echoMode: TextInput.Password; onTextChanged: p.setF("password", text) } }
        SRow { title: "Repite la contraseña"; Field { echoMode: TextInput.Password; onTextChanged: p.setF("password2", text) } }
        SRow { title: "Nombre del equipo"; separator: false; Field { text: p.f.hostname; onEditingFinished: p.setF("hostname", text) } }
    }

    // 4. resumen
    ColumnLayout {
        visible: Sys.live && p.step === 3 && !(p.status && p.status.started)
        spacing: 12
        Layout.fillWidth: true
        Card {
            SRow { title: "Disco"; subtitle: p.diskOf() ? p.diskOf().model + " (" + p.f.disk + ", " + p.diskOf().size_h + ")" : "" }
            SRow { title: "Cómo"; subtitle: p.f.mode === "wipe" ? "Borrando todo el disco" : "En el espacio libre" }
            SRow { title: "Cifrado"; subtitle: p.f.encrypt ? "Sí" : "No" }
            SRow { title: "Usuario"; subtitle: p.f.fullname + " · " + p.f.user + " · equipo " + p.f.hostname }
            SRow { title: "Idioma y teclado"; subtitle: p.f.locale + " · " + p.f.keymap + " · " + p.f.timezone; separator: false }
        }
        Card {
            visible: p.f.mode === "wipe"
            SRow {
                title: "Se borrará todo lo que hay en " + p.f.disk
                subtitle: "Entiendo que se perderán los datos de este disco"
                separator: false
                Toggle { checked: p.f.confirm; onToggled: function (on) { p.setF("confirm", on) } }
            }
        }
    }

    RowLayout {
        visible: Sys.live && !(p.status && p.status.started)
        Item { Layout.fillWidth: true }
        Btn { text: "Atrás"; visible: p.step > 0; onClicked: p.step-- }
        Btn { text: p.step === 3 ? "Instalar ahora" : "Siguiente"; kind: "primary"; onClicked: p.next() }
    }

    // progreso
    Card {
        visible: p.status !== null && p.status.started === true
        SRow {
            title: p.status && p.status.done ? (p.status.ok ? "Lily está instalado" : "La instalación no se ha completado") : (p.status && p.status.text ? p.status.text : "Empezando…")
            subtitle: p.status && p.status.done ? (p.status.ok ? "Reinicia y quita la memoria USB (o la ISO)." : p.status.error) : (p.status && p.status.steps ? "Paso " + p.status.step + " de " + p.status.steps : "")
            Btn { visible: p.status && p.status.done && p.status.ok; text: "Reiniciar ahora"; kind: "primary"; onClicked: Ui.api("POST", "/api/reboot", {}, function (r) { if (!r.ok) Ui.toast(r.error, true) }) }
        }
        Rectangle {
            Layout.fillWidth: true; Layout.margins: 16; height: 8; radius: 4; color: Ui.track
            Rectangle { height: 8; radius: 4; color: Ui.accent; width: parent.width * (p.status && p.status.steps ? (p.status.done && p.status.ok ? 1 : (p.status.step - 1) / p.status.steps) : 0) }
        }
        Txt {
            Layout.fillWidth: true; Layout.margins: 16; Layout.topMargin: 0
            text: p.status && p.status.log ? p.status.log.split("\n").slice(-8).join("\n") : ""
            color: Ui.muted; font.family: "JetBrains Mono"; font.pixelSize: 11; wrapMode: Text.WrapAnywhere; elide: Text.ElideNone
        }
    }
}
