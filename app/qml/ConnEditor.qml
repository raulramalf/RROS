// Opciones de una conexión: IP automática o manual, DNS, MTU, MAC, medida y conexión automática
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

Popup {
    id: e
    property var con: null
    property string method: "auto"
    signal saved()
    modal: true
    anchors.centerIn: Overlay.overlay
    width: 480
    padding: 22
    background: Rectangle { radius: 14; color: Ui.card; border.color: Ui.line }
    Overlay.modal: Rectangle { color: "#66000000" }

    function open_(c, m) {
        con = c
        method = m["ipv4.method"] === "manual" ? "manual" : "auto"
        addr.text = m["ipv4.addresses"] || ""
        gw.text = m["ipv4.gateway"] || ""
        dns.text = (m["ipv4.dns"] || "").replace(/,/g, ", ")
        mtu.text = m["802-3-ethernet.mtu"] && m["802-3-ethernet.mtu"] !== "auto" ? m["802-3-ethernet.mtu"] : ""
        metered.checked = m["connection.metered"] === "yes"
        auto.checked = m["connection.autoconnect"] !== "no"
        mac.value = m["802-11-wireless.cloned-mac-address"] === "random" ? "random" : m["802-11-wireless.cloned-mac-address"] === "stable" ? "stable" : "permanent"
        open()
    }
    function save() {
        var a = ["connection", "modify", con.uuid, "ipv4.method", method]
        if (method === "manual") a = a.concat(["ipv4.addresses", addr.text, "ipv4.gateway", gw.text])
        else a = a.concat(["ipv4.addresses", "", "ipv4.gateway", ""])
        a = a.concat(["ipv4.dns", dns.text.replace(/\s/g, ""), "ipv4.ignore-auto-dns", dns.text.trim() ? "yes" : "no"])
        a = a.concat(["connection.metered", metered.checked ? "yes" : "unknown", "connection.autoconnect", auto.checked ? "yes" : "no"])
        if (con.type === "802-3-ethernet") a = a.concat(["802-3-ethernet.mtu", mtu.text.trim() || "auto"])
        if (con.type === "802-11-wireless") a = a.concat(["802-11-wireless.cloned-mac-address", mac.value])
        Ui.run(["nmcli"].concat(a), function (r) {
            if (r.code !== 0) { Ui.toast((r.err || "No se pudo guardar").trim(), true); return }
            // se vuelve a activar para que los cambios se apliquen ya
            if (con.dev) Ui.run(["nmcli", "connection", "up", con.uuid], function () { e.saved() })
            else e.saved()
            Ui.toast("Guardado", false)
            e.close()
        })
    }

    contentItem: ColumnLayout {
        spacing: 10
        Txt { text: e.con ? e.con.name : ""; font.pixelSize: 17; font.weight: Font.Bold }
        RowLayout {
            Txt { text: "IPv4"; Layout.fillWidth: true }
            Seg { options: [["auto", "Automática (DHCP)"], ["manual", "Manual"]]; value: e.method; onPicked: function (v) { e.method = v } }
        }
        GridLayout {
            columns: 2; columnSpacing: 10; rowSpacing: 8
            visible: e.method === "manual"
            Txt { text: "Dirección/prefijo"; color: Ui.muted }
            Field { id: addr; Layout.fillWidth: true; placeholderText: "192.168.1.50/24" }
            Txt { text: "Puerta de enlace"; color: Ui.muted }
            Field { id: gw; Layout.fillWidth: true; placeholderText: "192.168.1.1" }
        }
        GridLayout {
            columns: 2; columnSpacing: 10; rowSpacing: 8
            Txt { text: "DNS"; color: Ui.muted }
            Field { id: dns; Layout.fillWidth: true; placeholderText: "vacío: los de la red" }
            Txt { text: "MTU"; color: Ui.muted; visible: e.con && e.con.type === "802-3-ethernet" }
            Field { id: mtu; Layout.fillWidth: true; placeholderText: "automática"; visible: e.con && e.con.type === "802-3-ethernet" }
        }
        RowLayout {
            visible: e.con && e.con.type === "802-11-wireless"
            Txt { text: "MAC"; Layout.fillWidth: true }
            Seg { id: mac; options: [["permanent", "Real"], ["stable", "Fija por red"], ["random", "Aleatoria"]] }
        }
        RowLayout { Txt { text: "Conexión de datos limitados"; Layout.fillWidth: true } Toggle { id: metered } }
        RowLayout { Txt { text: "Conectar automáticamente"; Layout.fillWidth: true } Toggle { id: auto } }
        RowLayout {
            Layout.topMargin: 6
            Item { Layout.fillWidth: true }
            Btn { text: "Cancelar"; onClicked: e.close() }
            Btn { text: "Guardar"; kind: "primary"; onClicked: e.save() }
        }
    }
}
