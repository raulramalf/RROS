// Avanzado: todas las claves de rros-config por categoría, copias de los ajustes y restablecer
import QtQuick
import QtQuick.Layouts
import "."

RPage {
    id: p
    title: "Avanzado"
    subtitle: "Todos los ajustes de Lily, uno a uno. Lo mismo que «lily config» en la terminal."
    icon: "qrc:/icons/sec-avanzado.svg"
    property string cat: "ui"
    property string q: ""

    Ask { id: ask }

    Section { text: "Copias de tus ajustes" }
    Card {
        Repeater {
            model: Ui.st ? Ui.st.snapshots : []
            SRow {
                title: modelData
                RowLayout {
                    Btn { text: "Restaurar"; onClicked: Ui.api("POST", "/api/snapshot", { action: "restore", name: modelData }, function (r) { Ui.toast(r.ok ? "Ajustes restaurados" : r.error, !r.ok); Ui.reload() }) }
                    Btn { text: "Borrar"; kind: "danger"; onClicked: Ui.api("POST", "/api/snapshot", { action: "delete", name: modelData }, function (r) { Ui.reload() }) }
                }
            }
        }
        SRow {
            title: "Guardar una copia"
            subtitle: "Para volver a tus ajustes de ahora cuando quieras"
            Btn {
                text: "Guardar…"
                onClicked: ask.ask("Copia de los ajustes", "", [{ id: "n", label: "Nombre", value: Qt.formatDate(new Date(), "yyyy-MM-dd") }], "Guardar",
                                   function (v) { Ui.api("POST", "/api/snapshot", { action: "save", name: v.n }, function (r) { Ui.toast(r.ok ? "Copia guardada" : r.error, !r.ok); Ui.reload() }) })
            }
        }
        SRow {
            title: "Volver a los ajustes de fábrica"
            subtitle: "Borra tus cambios (antes se guarda una copia «antes-de-restablecer»)"
            separator: false
            Btn {
                text: "Restablecer"; kind: "danger"
                onClicked: Ui.api("POST", "/api/snapshot", { action: "save", name: "antes-de-restablecer" }, function () {
                    Ui.run(["rros-config", "reset", "--yes"], function (r) { Ui.toast(r.code === 0 ? "Ajustes de fábrica" : r.err, r.code !== 0); Ui.reload(function () { Sys.detached(["rros-theme", "apply", "--quiet"]) }) })
                })
            }
        }
    }

    Section { text: "Todas las claves" }
    RowLayout {
        Layout.fillWidth: true
        Combo {
            options: Ui.st ? Ui.st.categories.map(function (c) { return [c.id, c.label] }) : []
            value: p.cat
            onPicked: function (v) { p.cat = v }
        }
        Field { Layout.fillWidth: true; placeholderText: "Filtrar (p. ej. dock, blur, firewall)"; onTextChanged: p.q = text.toLowerCase() }
    }
    Card {
        Repeater {
            model: Ui.st ? Ui.st.keys.filter(function (k) {
                return p.q !== "" ? (k.key + " " + k.desc).toLowerCase().indexOf(p.q) >= 0 : k.category === p.cat
            }) : []
            KeyRow {
                k: modelData.key
                title: modelData.key
                subtitle: modelData.desc + (modelData.source === "user" ? " · cambiado por ti" : modelData.source === "system" ? " · del sistema" : "") +
                          (modelData.scope === "system" ? " · pide contraseña" : "")
            }
        }
    }
}
