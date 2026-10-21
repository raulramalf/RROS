// Red e Internet: Wi-Fi, cable, conexiones guardadas (IP, DNS, MTU...), VPN, punto de acceso y proxy.
// Todo con NetworkManager (nmcli), sin contraseña de administrador en una sesión local.
import QtQuick
import QtQuick.Layouts
import "."

RPage {
    id: p
    title: "Red e Internet"
    subtitle: "Wi-Fi, cable, VPN y opciones de cada conexión"
    icon: "qrc:/icons/sec-red.svg"
    property bool wifiOn: true
    property bool airplane: false
    property var devices: []        // [{dev, type, state, con}]
    property var nets: []           // [{inuse, ssid, signal, sec}]
    property var cons: []           // [{name, uuid, type, dev, auto}]
    property var detail: ({})       // dev -> {ip, gw, dns, mac}
    property bool scanning: false
    property string proxyType: "0"
    property string proxyHttp: ""
    property bool nm: Sys.hasProgram("nmcli")

    function refresh() {
        if (!nm) return
        Ui.run(["nmcli", "-t", "-f", "WIFI,WWAN", "radio"], function (r) {
            var f = Ui.splitT(r.out.trim()); wifiOn = f[0] === "enabled"; airplane = f[0] !== "enabled" && f[1] !== "enabled"
        })
        Ui.run(["nmcli", "-t", "-f", "DEVICE,TYPE,STATE,CONNECTION", "device"], function (r) {
            var list = []
            Ui.lines(r.out).forEach(function (l) {
                var f = Ui.splitT(l)
                if (f[1] === "loopback" || f[1] === "bridge" || f[1].indexOf("p2p") >= 0) return
                list.push({ dev: f[0], type: f[1], state: f[2], con: f[3] })
                if (f[2] === "connected") devInfo(f[0])
            })
            devices = list
        })
        Ui.run(["nmcli", "-t", "-f", "NAME,UUID,TYPE,DEVICE,AUTOCONNECT", "connection", "show"], function (r) {
            cons = Ui.lines(r.out).map(function (l) { var f = Ui.splitT(l); return { name: f[0], uuid: f[1], type: f[2], dev: f[3], auto: f[4] } })
                                   .filter(function (c) { return c.type !== "loopback" && c.type !== "bridge" })
        })
        scan(false)
        proxyType = "0"
        Ui.run(["kreadconfig6", "--file", "kioslaverc", "--group", "Proxy Settings", "--key", "ProxyType"], function (r) { proxyType = r.out.trim() || "0" })
        Ui.run(["kreadconfig6", "--file", "kioslaverc", "--group", "Proxy Settings", "--key", "httpProxy"], function (r) { proxyHttp = r.out.trim().replace(" ", ":") })
    }
    function devInfo(dev) {
        Ui.run(["nmcli", "-t", "-f", "IP4.ADDRESS,IP4.GATEWAY,IP4.DNS,GENERAL.HWADDR,IP6.ADDRESS", "device", "show", dev], function (r) {
            var d = { ip: [], gw: "", dns: [], mac: "", ip6: [] }
            Ui.lines(r.out).forEach(function (l) {
                var i = l.indexOf(":"), k = l.slice(0, i), v = l.slice(i + 1).replace(/\\:/g, ":")
                if (k.indexOf("IP4.ADDRESS") === 0) d.ip.push(v)
                else if (k === "IP4.GATEWAY") d.gw = v
                else if (k.indexOf("IP4.DNS") === 0) d.dns.push(v)
                else if (k === "GENERAL.HWADDR") d.mac = v
                else if (k.indexOf("IP6.ADDRESS") === 0 && v.indexOf("fe80") !== 0) d.ip6.push(v)
            })
            var x = detail; x[dev] = d; detail = x
        })
    }
    function scan(rescan) {
        scanning = true
        Ui.run(["nmcli", "-t", "-f", "IN-USE,SSID,SIGNAL,SECURITY", "device", "wifi", "list", "--rescan", rescan ? "yes" : "auto"], function (r) {
            var seen = {}, list = []
            Ui.lines(r.out).forEach(function (l) {
                var f = Ui.splitT(l)
                if (!f[1] || seen[f[1]]) return
                seen[f[1]] = 1
                list.push({ inuse: f[0] === "*", ssid: f[1], signal: parseInt(f[2]) || 0, sec: f[3] && f[3] !== "--" ? f[3] : "" })
            })
            list.sort(function (a, b) { return (b.inuse - a.inuse) || (b.signal - a.signal) })
            nets = list
            scanning = false
        })
    }
    function nm_(args, okMsg) {
        Ui.run(["nmcli"].concat(args), function (r) {
            if (r.code !== 0) Ui.toast((r.err || r.out || "Falló").trim(), true)
            else if (okMsg) Ui.toast(okMsg, false)
            refresh()
        })
    }
    function connect(n) {
        var saved = cons.some(function (c) { return c.name === n.ssid })
        if (saved || !n.sec) { nm_(["device", "wifi", "connect", n.ssid], "Conectado a " + n.ssid); return }
        ask.ask("Conectarse a «" + n.ssid + "»", "Red protegida (" + n.sec + ")",
                [{ id: "pw", label: "Contraseña", password: true }], "Conectar",
                function (v) { nm_(["device", "wifi", "connect", n.ssid, "password", v.pw], "Conectado a " + n.ssid) })
    }
    function edit(c) {
        Ui.run(["nmcli", "-t", "-f", "ipv4.method,ipv4.addresses,ipv4.gateway,ipv4.dns,connection.metered,connection.autoconnect,802-3-ethernet.mtu,802-11-wireless.cloned-mac-address", "connection", "show", c.uuid], function (r) {
            var m = {}
            Ui.lines(r.out).forEach(function (l) { var i = l.indexOf(":"); m[l.slice(0, i)] = l.slice(i + 1).replace(/\\:/g, ":").replace(/^--$/, "") })
            editor.open_(c, m)
        })
    }
    onShown: refresh()

    Ask { id: ask }

    Note { visible: !p.nm; text: "NetworkManager no está instalado: la red no se puede configurar desde aquí."; kind: "warn" }

    Card {
        SRow {
            title: "Wi-Fi"
            subtitle: p.wifiOn ? (p.nets.length ? p.nets.length + " redes cerca" : "Buscando redes…") : "Apagado"
            Toggle { checked: p.wifiOn; onToggled: function (on) { p.nm_(["radio", "wifi", on ? "on" : "off"]) } }
        }
        SRow {
            title: "Modo avión"
            subtitle: "Apaga el Wi-Fi, el Bluetooth y la red móvil"
            separator: false
            Toggle { checked: p.airplane; onToggled: function (on) { Ui.run(["rfkill", on ? "block" : "unblock", "all"], function () { p.refresh() }) } }
        }
    }

    Section { text: "Conectado" }
    Card {
        Repeater {
            model: p.devices.filter(function (d) { return d.state === "connected" })
            SRow {
                property var d: p.detail[modelData.dev] || { ip: [], dns: [], gw: "", mac: "", ip6: [] }
                title: (modelData.type === "wifi" ? "Wi-Fi · " : modelData.type === "ethernet" ? "Cable · " : modelData.type + " · ") + modelData.con
                subtitle: "IP " + d.ip.join(", ") + (d.gw ? " · puerta de enlace " + d.gw : "") + (d.dns.length ? "\nDNS " + d.dns.join(", ") : "") +
                          (d.ip6.length ? "\nIPv6 " + d.ip6.join(", ") : "") + (d.mac ? "\nMAC " + d.mac + " · " + modelData.dev : "")
                RowLayout {
                    Btn { text: "Opciones"; onClicked: { var c = p.cons.filter(function (x) { return x.name === modelData.con })[0]; if (c) p.edit(c) } }
                    Btn { text: "Desconectar"; onClicked: p.nm_(["device", "disconnect", modelData.dev]) }
                }
            }
        }
        SRow { visible: !p.devices.some(function (d) { return d.state === "connected" }); title: "Sin conexión"; subtitle: "Elige una red Wi-Fi o conecta un cable"; separator: false }
    }

    Section { text: "Redes Wi-Fi"; visible: p.wifiOn }
    Card {
        visible: p.wifiOn
        Repeater {
            model: p.nets
            SRow {
                title: modelData.ssid
                subtitle: (modelData.inuse ? "Conectada · " : "") + (modelData.sec ? "Protegida (" + modelData.sec + ")" : "Abierta")
                clickable: !modelData.inuse
                onClicked: p.connect(modelData)
                RowLayout {
                    spacing: 10
                    Txt { text: modelData.sec ? "🔒" : ""; font.pixelSize: 11 }
                    Row {
                        spacing: 2
                        Repeater {
                            model: 4
                            Rectangle {
                                width: 4; height: 5 + index * 3; anchors.bottom: parent.bottom; radius: 1
                                color: modelData.signal > index * 25 ? Ui.textColor : Ui.track
                            }
                        }
                    }
                }
            }
        }
        SRow {
            title: p.scanning ? "Buscando…" : "Otra red…"
            subtitle: "Una red oculta (que no anuncia su nombre)"
            separator: false
            RowLayout {
                Btn { text: "Buscar"; busy: p.scanning; onClicked: p.scan(true) }
                Btn {
                    text: "Red oculta"
                    onClicked: ask.ask("Red oculta", "Escribe el nombre exacto de la red y su contraseña",
                                       [{ id: "ssid", label: "Nombre (SSID)" }, { id: "pw", label: "Contraseña (vacía si es abierta)", password: true }], "Conectar",
                                       function (v) { var a = ["device", "wifi", "connect", v.ssid, "hidden", "yes"]; if (v.pw) a = a.concat(["password", v.pw]); p.nm_(a, "Conectado") })
                }
            }
        }
    }

    Section { text: "Conexiones guardadas" }
    Card {
        Repeater {
            model: p.cons
            SRow {
                title: modelData.name
                subtitle: ({ "802-11-wireless": "Wi-Fi", "802-3-ethernet": "Cable", "vpn": "VPN", "wireguard": "WireGuard", "gsm": "Móvil" }[modelData.type] || modelData.type) +
                          (modelData.dev ? " · en uso" : "") + (modelData.auto === "yes" ? " · automática" : "")
                RowLayout {
                    Btn { text: modelData.dev ? "Desactivar" : "Activar"; onClicked: p.nm_(["connection", modelData.dev ? "down" : "up", modelData.uuid]) }
                    Btn { text: "Opciones"; onClicked: p.edit(modelData) }
                    Btn { text: "Olvidar"; kind: "danger"; onClicked: p.nm_(["connection", "delete", modelData.uuid], "Conexión olvidada") }
                }
            }
        }
        SRow {
            title: "VPN"
            subtitle: "Importa un archivo de OpenVPN (.ovpn) o de WireGuard (.conf)"
            separator: false
            Btn {
                text: "Importar…"
                onClicked: ask.ask("Importar VPN", "Ruta del archivo de configuración", [{ id: "f", label: "Archivo", placeholder: Sys.home + "/Descargas/vpn.ovpn" }], "Importar",
                                   function (v) { p.nm_(["connection", "import", "type", v.f.match(/\.ovpn$/) ? "openvpn" : "wireguard", "file", v.f], "VPN importada") })
            }
        }
    }

    Section { text: "Compartir" }
    Card {
        SRow {
            title: "Punto de acceso Wi-Fi"
            subtitle: "Comparte la conexión por cable con tus otros aparatos"
            separator: false
            Btn {
                text: "Crear…"
                onClicked: ask.ask("Punto de acceso", "Los demás se conectan a esta red", [{ id: "ssid", label: "Nombre", value: "Lily" }, { id: "pw", label: "Contraseña (8 caracteres o más)", password: true }], "Crear",
                                   function (v) { p.nm_(["device", "wifi", "hotspot", "ssid", v.ssid, "password", v.pw], "Punto de acceso «" + v.ssid + "» activo") })
            }
        }
    }

    Section { text: "Avanzado" }
    Card {
        SRow {
            title: "Proxy"
            subtitle: p.proxyType === "1" ? "Manual: " + p.proxyHttp : "Sin proxy (conexión directa)"
            RowLayout {
                Btn { text: "Quitar"; visible: p.proxyType === "1"; onClicked: Ui.run(["kwriteconfig6", "--file", "kioslaverc", "--group", "Proxy Settings", "--key", "ProxyType", "0"], function () { p.refresh() }) }
                Btn {
                    text: "Configurar…"
                    onClicked: ask.ask("Proxy", "Para el navegador y las aplicaciones de KDE", [{ id: "h", label: "Servidor:puerto", placeholder: "proxy.ejemplo.com:3128", value: p.proxyHttp }], "Guardar",
                                       function (v) {
                                           var hp = v.h.replace(":", " ")
                                           Ui.run(["sh", "-c", "kwriteconfig6 --file kioslaverc --group 'Proxy Settings' --key ProxyType 1 && " +
                                                   "for k in httpProxy httpsProxy ftpProxy; do kwriteconfig6 --file kioslaverc --group 'Proxy Settings' --key $k \"$1\"; done", "rros", "http://" + hp],
                                                  function () { p.refresh() })
                                       })
                }
            }
        }
        KeyRow { k: "net.dns"; title: "Servidores DNS del sistema"; subtitle: "Separados por comas, p. ej. 1.1.1.1, 9.9.9.9 (vacío: los de cada red)" }
        KeyRow { k: "net.mac_randomize"; title: "Dirección MAC aleatoria"; subtitle: "Más privacidad en redes Wi-Fi públicas" }
        KeyRow { k: "net.wifi_powersave"; title: "Ahorro de energía del Wi-Fi"; separator: false }
    }

    // ---- opciones de una conexión ----
    ConnEditor { id: editor; onSaved: p.refresh() }
}
