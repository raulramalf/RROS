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
    property string tag: ""          // etiqueta elegida dentro de la categoría (navegador, fotos, emuladores…)
    property bool loadingApps: false
    property bool loadFailed: false
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
                                 ["graphics", "Gráficos"], ["dev", "Programación"], ["utility", "Utilidades"], ["games", "Juegos"],
                                 ["education", "Educación"], ["system", "Sistema"]]
    readonly property var srcName: ({ "rros": "Probada en Lily", "arch": "Repositorios de Arch", "flathub": "Flathub" })

    // el catálogo: por la API de Ajustes y, si falla o llega vacío, directamente de rros-apps; si tampoco, se dice
    // (con «Reintentar») en vez de dejar la Tienda en blanco
    function refresh() {
        loadingApps = true; loadFailed = false
        Ui.api("GET", "/api/apps", {}, function (r) {
            if (Array.isArray(r) && r.length > 0) { apps = r; loadingApps = false; return }
            json(["list", "--json"], function (v) { p.apps = v; p.loadingApps = false; p.loadFailed = v.length === 0 })
        })
    }
    // las etiquetas más usadas de la categoría elegida, para afinar (máx. 16)
    readonly property var tags: {
        var n = {}
        apps.forEach(function (a) {
            if (cat !== "" && a.category !== cat) return
            ;(a.tags || "").split(",").forEach(function (t) { t = t.trim(); if (t) n[t] = (n[t] || 0) + 1 })
        })
        return Object.keys(n).sort(function (x, y) { return n[y] - n[x] || x.localeCompare(y) }).slice(0, 16)
    }
    function visibleApps() {
        return apps.filter(function (a) {
            return (cat === "" || a.category === cat) && (tag === "" || (a.tags || "").split(",").indexOf(tag) >= 0)
        }).map(catalogItem)
    }
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
    // progreso de lo que se instala: solo una frase y una barra, nunca las líneas de pacman o Flatpak (rutas, comandos)
    property real progress: -1      // 0..1, o -1 si aún no se sabe (barra en movimiento)
    property string lastLine: ""    // la última línea, solo para el aviso si falla
    function feed(line, install) {
        p.lastLine = line
        var m = /\(\s*(\d+)\s*\/\s*(\d+)\s*\)/.exec(line), pc = /(\d{1,3})\s?%/.exec(line)
        var phase = /descarg|download|retriev|recibiendo|fetching/i.test(line) ? "Descargando…"
                  : /instal|upgrad|actualiz|deploy/i.test(line) ? (install ? "Instalando…" : "Actualizando…")
                  : /remov|quitando|desinstal|borrando/i.test(line) ? "Quitando…"
                  : /check|comprob|verific|resolv|integr|conflict|key|clave/i.test(line) ? "Comprobando…" : ""
        if (phase !== "") p.log = phase
        // pacman: «( 3/12) instalando x»; Flatpak: «… 45%». La fase de descarga va de 0 a 60 %, la instalación de 60 a 100 %
        if (m) p.progress = Math.max(p.progress, 0.6 + 0.4 * Number(m[1]) / Math.max(1, Number(m[2])))
        else if (pc) p.progress = Math.max(p.progress, (p.log === "Descargando…" ? 0.6 : 1) * Math.min(100, Number(pc[1])) / 100)
    }
    function act(a, install) {
        var argv = a.origen === "rros" ? ["pkexec", "rros-apps", install ? "install" : "remove", a.id]
                 : ["pkexec", "rros-apps", (install ? "instalar-" : "quitar-") + (a.origen === "flathub" ? "flatpak" : "paquete"), a.id]
        working = a.origen + "/" + a.id; log = install ? "Preparando…" : "Quitando…"; progress = -1; lastLine = ""
        Sys.stream(argv, function (line) { p.feed(line, install) }, function (code) {
            p.working = ""; p.progress = -1
            if (code === 0) Ui.toast(a.name + (install ? " instalada" : " quitada"), false)
            else if (code === 126 || code === 127) Ui.toast("Hace falta la contraseña de un administrador", true)
            else Ui.toast("No se pudo " + (install ? "instalar " : "quitar ") + a.name + (p.lastLine ? ": " + p.lastLine.slice(0, 120) : ""), true)
            p.refresh(); if (p.q.length >= 2) p.search(); p.loadTab()
            if (p.detail && p.detail.id === a.id) p.detail = Object.assign({}, p.detail, { installed: install && code === 0 ? true : (!install && code === 0 ? false : p.detail.installed) })
        })
    }
    function updateAll() {
        working = "*"; log = "Actualizando…"
        progress = -1
        Sys.stream(["pkexec", "rros-apps", "update"], function (l) { p.feed(l, false) }, function (code) {
            p.working = ""; p.progress = -1; Ui.toast(code === 0 ? "Todo al día" : "No se pudo actualizar", code !== 0); p.loadTab()
        })
    }
    // icono de la app: tema, hicolor, AppStream de Arch o de Flathub; si no hay, una ficha con su inicial
    function iconOf(a) {
        if (!a) return ""
        var pk = String(a.pacman || "").split(",")[0]
        return Sys.appIcon([a.cmd || "", a.icon || "", pk, a.flatpak || "", a.id || "", String(a.id || "").split(".").pop().toLowerCase()],
                           a.flatpak || (a.origen === "flathub" ? a.id : ""))
    }
    function catalogItem(a) { return { origen: "rros", id: a.id, name: a.name, desc: a.desc, version: "", icon: a.cmd || a.id, pacman: a.pacman || "", flatpak: a.flatpak || "", installed: a.installed, shipped: a.shipped, cmd: a.cmd, category: a.category, tags: a.tags || "" } }
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
                AppIco { app: p.detail; size: 96 }
                ColumnLayout {
                    spacing: 4
                    Layout.fillWidth: true
                    Txt { text: p.detail ? p.detail.name : ""; font.pixelSize: 22; font.weight: Font.Bold }
                    Txt { text: p.detail ? p.srcName[p.detail.origen] + (p.detail.version ? " · " + p.detail.version : "") : ""; color: Ui.accent; font.pixelSize: Ui.fs - 1 }
                    Txt { text: p.detail ? (p.working === p.detail.origen + "/" + p.detail.id ? p.log : p.detail.desc) : ""; color: Ui.muted; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    Bar { visible: p.detail !== null && p.working === p.detail.origen + "/" + p.detail.id; Layout.fillWidth: true; Layout.maximumWidth: 360 }
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
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { p.cat = modelData[0]; p.tag = "" } }
            }
        }
    }
    // etiquetas de la categoría: «navegador», «fotos», «emuladores»…
    Flow {
        visible: p.tab === "explorar" && p.q === "" && p.detail === null && p.tags.length > 0
        Layout.fillWidth: true
        spacing: 6
        Repeater {
            model: p.tags
            Rectangle {
                property bool on: p.tag === modelData
                implicitWidth: tt.implicitWidth + 20; implicitHeight: 24; radius: 12
                color: on ? Qt.rgba(Qt.color(Ui.accent).r, Qt.color(Ui.accent).g, Qt.color(Ui.accent).b, 0.25) : "transparent"
                border.color: on ? Ui.accent : Ui.line
                Txt { id: tt; anchors.centerIn: parent; text: "# " + modelData; font.pixelSize: Ui.fs - 2; color: on ? Ui.textColor : Ui.muted }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: p.tag = on ? "" : modelData }
            }
        }
    }
    // cargando o sin catálogo
    RowLayout {
        visible: p.tab === "explorar" && p.q === "" && p.detail === null && p.apps.length === 0
        spacing: 12
        Txt { text: p.loadingApps ? "Cargando el catálogo…" : "No se pudo leer el catálogo de aplicaciones."; color: Ui.muted }
        Btn { visible: !p.loadingApps; text: "Reintentar"; onClicked: p.refresh() }
    }
    // portada: la primera destacada en grande
    Rectangle {
        visible: p.tab === "explorar" && p.q === "" && p.cat === "" && p.tag === "" && p.detail === null && p.apps.length > 0
        Layout.fillWidth: true
        implicitHeight: 150
        radius: 16
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0; color: Qt.darker(Ui.accent, 1.6) }
            GradientStop { position: 1; color: Qt.darker(Ui.pal.cyan, 2.2) }
        }
        // una destacada distinta cada día, de las más conocidas que aún no tienes
        readonly property var picks: ["vlc", "gimp", "obs", "steam", "kdenlive", "blender", "spotify", "krita", "onlyoffice", "kodi",
                                      "retroarch", "stellarium", "inkscape", "telegram", "freecad", "heroic", "audacity", "darktable"]
        property var hero: {
            var cand = p.apps.filter(function (a) { return !a.installed && picks.indexOf(a.id) >= 0 })
            if (!cand.length) cand = p.apps.filter(function (a) { return !a.installed })
            if (!cand.length) cand = p.apps
            return cand.length ? p.catalogItem(cand[Math.floor(Date.now() / 86400000) % cand.length]) : null
        }
        RowLayout {
            anchors { fill: parent; margins: 24 }
            spacing: 22
            AppIco { app: parent.parent.hero; size: 96 }
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
                 : p.visibleApps()
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
                    AppIco { app: modelData; size: 56 }
                    ColumnLayout {
                        spacing: 2
                        Layout.fillWidth: true
                        Txt { text: modelData.name; font.weight: Font.DemiBold; Layout.fillWidth: true; elide: Text.ElideRight }
                        Txt { text: p.working === modelData.origen + "/" + modelData.id ? p.log
                                  : p.tab === "actualizaciones" && modelData.desc ? modelData.desc + "  →  " + modelData.version : modelData.desc
                              color: Ui.muted; font.pixelSize: Ui.fs - 1; wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight; Layout.fillWidth: true }
                        Bar { visible: p.working === modelData.origen + "/" + modelData.id; Layout.fillWidth: true }
                        RowLayout {
                            spacing: 6
                            Rectangle {
                                implicitWidth: sl.implicitWidth + 12; implicitHeight: 18; radius: 9
                                color: modelData.origen === "rros" ? Ui.accent : Ui.field
                                Txt { id: sl; anchors.centerIn: parent; text: p.srcName[modelData.origen] || ""; font.pixelSize: 10
                                      color: modelData.origen === "rros" ? Ui.accentText : Ui.muted }
                            }
                            Txt { visible: modelData.installed && p.tab === "explorar"; text: "Instalada"; color: Ui.ok; font.pixelSize: 11 }
                            Txt { visible: (modelData.tags || "") !== ""; text: (modelData.tags || "").split(",").slice(0, 2).map(function (t) { return "#" + t }).join("  ")
                                  color: Ui.muted; font.pixelSize: 10; elide: Text.ElideRight; Layout.fillWidth: true }
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

    // icono de una app; mientras carga o si no tiene, una ficha redondeada del color de su categoría con la inicial
    component AppIco: Item {
        id: ai
        property var app: null
        property int size: 56
        Layout.preferredWidth: size; Layout.preferredHeight: size
        implicitWidth: size; implicitHeight: size
        readonly property var tints: ({ internet: "#2f80ed", office: "#2e9e6a", media: "#d9304a", graphics: "#a855f7",
                                        dev: "#475569", games: "#f59e0b", utility: "#0ea5a4", system: "#64748b", education: "#16a34a" })
        readonly property color tint: (app && tints[app.category]) || Ui.accent
        Rectangle {
            anchors.fill: parent; anchors.margins: ai.size * 0.06
            radius: width * 0.24
            visible: ico.status !== Image.Ready
            gradient: Gradient {
                GradientStop { position: 0; color: Qt.lighter(ai.tint, 1.3) }
                GradientStop { position: 1; color: ai.tint }
            }
            Txt { anchors.centerIn: parent; color: "white"; font.pixelSize: parent.width * 0.48; font.weight: Font.Bold
                  text: ai.app && ai.app.name ? ai.app.name.charAt(0).toUpperCase() : "" }
        }
        Image {
            id: ico
            anchors.fill: parent
            source: p.iconOf(ai.app)
            sourceSize: Qt.size(ai.size * 2, ai.size * 2)
            fillMode: Image.PreserveAspectFit
            asynchronous: true; smooth: true; mipmap: true
        }
    }

    // barra de progreso: se llena con el porcentaje; si aún no lo hay, un tramo que va y viene
    component Bar: Rectangle {
        implicitHeight: 6; radius: 3; color: Ui.field; clip: true
        Rectangle {
            height: parent.height; radius: 3; color: Ui.accent
            width: p.progress >= 0 ? parent.width * p.progress : parent.width * 0.3
            Behavior on width { enabled: p.progress >= 0; NumberAnimation { duration: 250 } }
            SequentialAnimation on x {
                running: p.progress < 0 && parent.visible; loops: Animation.Infinite
                NumberAnimation { from: -parent.width * 0.3; to: parent.width; duration: 1200; easing.type: Easing.InOutQuad }
            }
            Binding on x { when: p.progress >= 0; value: 0 }
        }
        Txt { visible: p.progress >= 0; anchors { right: parent.right; bottom: parent.top; bottomMargin: 2 }
              text: Math.round(p.progress * 100) + " %"; font.pixelSize: 10; color: Ui.muted }
    }
}
