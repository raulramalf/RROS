// Tienda de Lily, al estilo de GNOME Software o Discover: Explorar (destacadas y categorías del catálogo probado de
// Lily), búsqueda en todo Flathub y en los repositorios de Arch, Instaladas y Actualizaciones. Instala con el progreso
// en la misma ventana; los paquetes con la contraseña de administrador y Flathub para todo el equipo.
import QtQuick
import QtQuick.Layouts
import "."

RPage {
    id: p
    title: "Tienda"
    subtitle: "Miles de aplicaciones: las probadas para Lily, todo Flathub y los repositorios de Arch"
    icon: "qrc:/icons/app-tienda.svg"
    property string tab: "explorar"
    property var apps: []            // catálogo de Lily
    property string cat: ""
    property string q: ""
    property var found: []
    property bool searching: false
    property var installed: []
    property var pending: []
    property bool loadingTab: false
    property string working: ""      // origen/id de lo que se instala o quita
    property string log: ""
    property var detail: null
    readonly property var cats: [["", "Destacadas"], ["internet", "Internet"], ["office", "Oficina"], ["media", "Música y vídeo"],
                                 ["graphics", "Gráficos"], ["dev", "Programación"], ["utility", "Utilidades"], ["games", "Juegos"], ["system", "Sistema"]]
    readonly property var srcName: ({ "rros": "Probada en Lily", "arch": "Repositorios de Arch", "flathub": "Flathub" })

    function refresh() { Ui.api("GET", "/api/apps", {}, function (r) { apps = Array.isArray(r) ? r : [] }) }
    function json(cmd, cb) { Sys.run(["rros-apps"].concat(cmd), function (r) { var v = []; try { v = JSON.parse(r.out) } catch (e) {} cb(v) }) }
    function search() {
        if (q.length < 2) { found = []; return }
        searching = true
        var asked = q
        json(["buscar", q], function (v) { if (asked === p.q) { p.found = v; p.searching = false } })
    }
    function loadTab() {
        if (tab === "instaladas") { loadingTab = true; json(["instaladas"], function (v) { p.installed = v; p.loadingTab = false }) }
        if (tab === "actualizaciones") { loadingTab = true; json(["pendientes"], function (v) { p.pending = v; p.loadingTab = false }) }
    }
    // a: { origen, id, name, ... }
    function act(a, install) {
        var argv = a.origen === "rros" ? ["pkexec", "rros-apps", install ? "install" : "remove", a.id]
                 : ["pkexec", "rros-apps", (install ? "instalar-" : "quitar-") + (a.origen === "flathub" ? "flatpak" : "paquete"), a.id]
        working = a.origen + "/" + a.id; log = install ? "Preparando…" : "Quitando…"
        Sys.stream(argv, function (line) { p.log = line }, function (code) {
            p.working = ""
            if (code === 0) Ui.toast(a.name + (install ? " instalada" : " quitada"), false)
            else if (code === 126 || code === 127) Ui.toast("Hace falta la contraseña de un administrador", true)
            else Ui.toast("No se pudo: " + p.log, true)
            p.refresh(); if (p.q.length >= 2) p.search(); p.loadTab()
            if (p.detail && p.detail.id === a.id) p.detail = Object.assign({}, p.detail, { installed: install && code === 0 ? true : (!install && code === 0 ? false : p.detail.installed) })
        })
    }
    function updateAll() {
        working = "*"; log = "Actualizando…"
        Sys.stream(["pkexec", "rros-apps", "update"], function (l) { p.log = l }, function (code) {
            p.working = ""; Ui.toast(code === 0 ? "Todo al día" : "No se pudo actualizar: " + p.log, code !== 0); p.loadTab()
        })
    }
    function iconOf(a) { return "image://icon/" + (a.icon || a.cmd || a.id) }
    function catalogItem(a) { return { origen: "rros", id: a.id, name: a.name, desc: a.desc, version: "", icon: a.installed ? (a.cmd || a.id) : (a.flatpak || a.cmd || a.id), installed: a.installed, shipped: a.shipped, cmd: a.cmd, category: a.category } }
    onShown: if (apps.length === 0) refresh()
    onTabChanged: loadTab()

    Timer { id: debounce; interval: 450; onTriggered: p.search() }

    Note {
        visible: Sys.live
        kind: "warn"
        text: "Estás en el sistema en vivo: lo que instales se guarda en la memoria y se pierde al apagar. Instala Lily para conservarlo."
    }
    // ---- pestañas y buscador ----
    RowLayout {
        Layout.fillWidth: true
        spacing: 12
        Seg {
            options: [["explorar", "Explorar"], ["instaladas", "Instaladas"], ["actualizaciones", "Actualizaciones"]]
            value: p.tab
            onPicked: function (v) { p.tab = v; p.detail = null }
        }
        Field {
            Layout.fillWidth: true
            visible: p.tab === "explorar"
            placeholderText: "Buscar en el catálogo, Flathub y Arch (p. ej. «fotos», «obs», «steam»)"
            onTextChanged: { p.q = text.trim(); p.detail = null; debounce.restart() }
        }
    }

    // ---- ficha de una aplicación ----
    Card {
        visible: p.detail !== null
        SRow {
            separator: false
            title: ""
            RowLayout {
                Layout.fillWidth: true
                spacing: 18
                Image { source: p.detail ? p.iconOf(p.detail) : ""; sourceSize: Qt.size(96, 96); Layout.preferredWidth: 96; Layout.preferredHeight: 96 }
                ColumnLayout {
                    spacing: 4
                    Layout.fillWidth: true
                    Txt { text: p.detail ? p.detail.name : ""; font.pixelSize: 22; font.weight: Font.Bold }
                    Txt { text: p.detail ? p.srcName[p.detail.origen] + (p.detail.version ? " · " + p.detail.version : "") : ""; color: Ui.accent; font.pixelSize: Ui.fs - 1 }
                    Txt { text: p.detail ? (p.working === p.detail.origen + "/" + p.detail.id ? p.log : p.detail.desc) : ""; color: Ui.muted; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                }
                ColumnLayout {
                    Btn {
                        text: p.detail && p.detail.installed ? "Abrir" : "Instalar"
                        kind: p.detail && p.detail.installed ? "normal" : "primary"
                        busy: p.detail !== null && p.working === p.detail.origen + "/" + p.detail.id
                        enabled2: p.working === ""
                        onClicked: p.detail.installed ? Sys.detached(p.detail.origen === "flathub" ? ["flatpak", "run", p.detail.id] : [p.detail.cmd || p.detail.id]) : p.act(p.detail, true)
                    }
                    Btn { visible: p.detail !== null && p.detail.installed && !p.detail.shipped; text: "Quitar"; kind: "danger"; enabled2: p.working === ""; onClicked: p.act(p.detail, false) }
                    Btn { text: "‹ Volver"; onClicked: p.detail = null }
                }
            }
        }
    }

    // ---- Explorar: categorías y destacadas (catálogo probado) ----
    Flow {
        visible: p.tab === "explorar" && p.q === "" && p.detail === null
        Layout.fillWidth: true
        spacing: 6
        Repeater {
            model: p.cats.filter(function (c) { return c[0] === "" || p.apps.some(function (a) { return a.category === c[0] }) })
            Rectangle {
                property bool on: p.cat === modelData[0]
                implicitWidth: ct.implicitWidth + 24; implicitHeight: 30; radius: 15
                color: on ? Ui.accent : Ui.field
                border.color: on ? "transparent" : Ui.line
                Txt { id: ct; anchors.centerIn: parent; text: modelData[1]; color: on ? Ui.accentText : Ui.textColor; font.pixelSize: Ui.fs - 1 }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: p.cat = modelData[0] }
            }
        }
    }
    // portada: la primera destacada en grande
    Rectangle {
        visible: p.tab === "explorar" && p.q === "" && p.cat === "" && p.detail === null && p.apps.length > 0
        Layout.fillWidth: true
        implicitHeight: 150
        radius: 16
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0; color: Qt.darker(Ui.accent, 1.6) }
            GradientStop { position: 1; color: Qt.darker(Ui.pal.cyan, 2.2) }
        }
        property var hero: p.apps.length ? p.catalogItem(p.apps.filter(function (a) { return !a.installed })[0] || p.apps[0]) : null
        RowLayout {
            anchors { fill: parent; margins: 24 }
            spacing: 22
            Image { source: parent.parent.hero ? p.iconOf(parent.parent.hero) : ""; sourceSize: Qt.size(96, 96); Layout.preferredWidth: 96; Layout.preferredHeight: 96 }
            ColumnLayout {
                Layout.fillWidth: true
                Txt { text: "Destacada"; color: "#ffffffcc"; font.pixelSize: Ui.fs - 1; font.weight: Font.DemiBold }
                Txt { text: parent.parent.parent.hero ? parent.parent.parent.hero.name : ""; color: "white"; font.pixelSize: 26; font.weight: Font.Bold }
                Txt { text: parent.parent.parent.hero ? parent.parent.parent.hero.desc : ""; color: "#ffffffdd"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
            }
        }
        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: p.detail = parent.hero }
    }

    // ---- cuadrícula de tarjetas (catálogo, resultados, instaladas, actualizaciones) ----
    Txt {
        visible: p.tab === "explorar" && p.q !== "" && p.detail === null
        text: p.searching ? "Buscando en el catálogo, Flathub y Arch…" : p.q.length < 2 ? "Escribe al menos dos letras" : p.found.length + " resultados"
        color: Ui.muted
    }
    Txt {
        visible: p.tab !== "explorar" && p.detail === null
        text: p.loadingTab ? "Cargando…" : p.tab === "instaladas" ? p.installed.length + " aplicaciones instaladas"
            : p.pending.length === 0 ? "Todo está al día" : p.pending.length + " actualizaciones disponibles"
        color: Ui.muted
    }
    Btn {
        visible: p.tab === "actualizaciones" && p.pending.length > 0 && p.detail === null
        text: "Actualizar todo"; kind: "primary"; busy: p.working === "*"; enabled2: p.working === ""
        onClicked: p.updateAll()
    }
    GridLayout {
        visible: p.detail === null
        Layout.fillWidth: true
        columns: width > 900 ? 3 : width > 600 ? 2 : 1
        columnSpacing: 12
        rowSpacing: 12
        Repeater {
            model: p.detail !== null ? []
                 : p.tab === "instaladas" ? p.installed
                 : p.tab === "actualizaciones" ? p.pending
                 : p.q !== "" ? p.found
                 : p.apps.filter(function (a) { return p.cat === "" || a.category === p.cat }).map(p.catalogItem)
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 96
                radius: 14
                color: cardMa.containsMouse ? Ui.cardHover : Ui.card
                border.color: Ui.line
                MouseArea { id: cardMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: p.detail = modelData }
                RowLayout {
                    anchors { fill: parent; margins: 14 }
                    spacing: 14
                    Image { source: p.iconOf(modelData); sourceSize: Qt.size(56, 56); Layout.preferredWidth: 56; Layout.preferredHeight: 56; asynchronous: true }
                    ColumnLayout {
                        spacing: 2
                        Layout.fillWidth: true
                        Txt { text: modelData.name; font.weight: Font.DemiBold; Layout.fillWidth: true; elide: Text.ElideRight }
                        Txt { text: p.working === modelData.origen + "/" + modelData.id ? p.log
                                  : p.tab === "actualizaciones" && modelData.desc ? modelData.desc + "  →  " + modelData.version : modelData.desc
                              color: Ui.muted; font.pixelSize: Ui.fs - 1; wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight; Layout.fillWidth: true }
                        RowLayout {
                            spacing: 6
                            Rectangle {
                                implicitWidth: sl.implicitWidth + 12; implicitHeight: 18; radius: 9
                                color: modelData.origen === "rros" ? Ui.accent : Ui.field
                                Txt { id: sl; anchors.centerIn: parent; text: p.srcName[modelData.origen] || ""; font.pixelSize: 10
                                      color: modelData.origen === "rros" ? Ui.accentText : Ui.muted }
                            }
                            Txt { visible: modelData.installed && p.tab === "explorar"; text: "Instalada"; color: Ui.ok; font.pixelSize: 11 }
                        }
                    }
                    Btn {
                        visible: !modelData.installed
                        text: "Instalar"; kind: "primary"
                        busy: p.working === modelData.origen + "/" + modelData.id
                        enabled2: p.working === ""
                        onClicked: p.act(modelData, true)
                    }
                }
            }
        }
    }
}
