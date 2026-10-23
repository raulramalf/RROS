// Apariencia: tema, claro u oscuro, color de acento, fondo y letra
import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import "."

RPage {
    id: p
    title: "Apariencia"
    subtitle: "Colores, fondo de pantalla y letra. Todo el sistema cambia a la vez: ventanas, barra, dock y terminal."
    icon: "qrc:/icons/sec-apariencia.svg"
    property var previews: ({})
    // miniatura de un fondo generado: la hecha con tus colores si ya está, si no la que trae el sistema
    function stylePreview(st) { return previews[st] || (st !== "liso" ? "file:///usr/share/rros/estilos/" + st + ".png" : "") }

    function useTheme(name) {
        Ui.run(["rros-theme", "use", name], function (r) {
            if (r.code !== 0) Ui.toast(r.err || "No se pudo cambiar el tema", true)
            Ui.reload()
        })
    }
    onShown: {
        var st = ["aurora", "ondas", "degradado", "circulos", "petalos", "dunas", "malla", "cristal", "topografico", "seda"]
        st.forEach(function (s) {
            Ui.api("GET", "/api/wallpaper_preview?style=" + s, {}, function (r) {
                if (r && r.ok) { var x = p.previews; x[s] = "file://" + r.path + "?" + Date.now(); p.previews = x }
            })
        })
    }

    Section { text: "Modo" }
    Card {
        SRow {
            title: "Aspecto"
            subtitle: "Claro u oscuro"
            separator: false
            RowLayout {
                spacing: 14
                Repeater {
                    model: [["light", "Claro", "#f6efe7", "#ffffff", "#2a0f16"], ["dark", "Oscuro", "#140609", "#2a1018", "#f6ecdf"]]
                    ColumnLayout {
                        spacing: 6
                        Rectangle {
                            width: 96; height: 62; radius: 10
                            color: modelData[2]
                            border.width: Ui.v("ui.color_scheme") === modelData[0] ? 3 : 1
                            border.color: Ui.v("ui.color_scheme") === modelData[0] ? Ui.accent : Ui.line
                            Rectangle { x: 10; y: 10; width: 60; height: 34; radius: 6; color: modelData[3] }
                            Rectangle { x: 18; y: 18; width: 30; height: 5; radius: 2; color: modelData[4]; opacity: .6 }
                            Rectangle { x: 18; y: 28; width: 22; height: 5; radius: 2; color: Ui.accent }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: Ui.setKey("ui.color_scheme", modelData[0]) }
                        }
                        Txt { text: modelData[1]; Layout.alignment: Qt.AlignHCenter; font.pixelSize: Ui.fs - 1 }
                    }
                }
            }
        }
    }

    Section { text: "Color de acento" }
    Card {
        SRow {
            title: "Acento"
            subtitle: "Botones, selección, interruptores y el borde de la ventana activa"
            separator: false
            RowLayout {
                spacing: 8
                Repeater {
                    model: ["", "#c8243c", "#d9b98f", "#7c5cff", "#0a84ff", "#22c3e6", "#30d158", "#ffb020", "#ff6b3d", "#ff375f", "#bf5af2"]
                    Rectangle {
                        property bool on: modelData === "" ? Ui.v("ui.accent") === "" : Ui.v("ui.accent").toLowerCase() === modelData
                        width: 24; height: 24; radius: 12
                        color: modelData === "" ? Ui.pal.accent : modelData
                        border.width: on ? 3 : 0
                        border.color: Ui.textColor
                        Txt { anchors.centerIn: parent; visible: modelData === ""; text: "T"; font.pixelSize: 10; font.weight: Font.Bold; color: Ui.accentText }
                        MouseArea {
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: modelData === "" ? Ui.unsetKey("ui.accent") : Ui.setKey("ui.accent", modelData)
                        }
                    }
                }
            }
        }
    }

    Section { text: "Tema" }
    Flow {
        Layout.fillWidth: true
        spacing: 12
        Repeater {
            model: Ui.st ? Ui.st.themes : []
            Rectangle {
                property var c: Ui.dark ? modelData.dark : modelData.light
                property bool on: Ui.st.current.theme === modelData.name
                width: 160; height: 112; radius: 12
                color: c.bg
                border.width: on ? 3 : 1
                border.color: on ? Ui.accent : Ui.line
                Rectangle {
                    x: 12; y: 12; width: 136; height: 52; radius: 8; color: c.surface
                    Row {
                        anchors.centerIn: parent
                        spacing: 6
                        Repeater {
                            model: [c.accent, c.red, c.green, c.yellow, c.blue]
                            Rectangle { width: 18; height: 18; radius: 9; color: modelData }
                        }
                    }
                }
                Txt { x: 14; y: 72; text: modelData.name.charAt(0).toUpperCase() + modelData.name.slice(1); color: c.text; font.weight: Font.DemiBold }
                Txt { x: 14; y: 90; text: modelData.builtin ? "Incluido" : "Propio"; color: c.muted; font.pixelSize: 10 }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: p.useTheme(modelData.name) }
            }
        }
    }

    Section { text: "Fondo de pantalla" }
    // vista previa: el escritorio en pequeño con la barra de tareas. Al pasar el ratón por un fondo se ve aquí
    // antes de elegirlo; con un clic se pone.
    property string hoverWall: ""
    readonly property string curWall: Ui.v("ui.wallpaper") !== "" ? "file://" + Ui.v("ui.wallpaper") : p.stylePreview(Ui.v("ui.wallpaper_style"))
    property var lilyWalls: []
    function loadWalls() {
        var dir = "/usr/share/wallpapers/Lily"
        p.lilyWalls = Sys.readFile(dir + "/fondos.txt").split("\n").filter(function (l) { return l !== "" }).map(function (l) {
            var x = l.split("|"); return { path: dir + "/" + x[0], thumb: dir + "/miniaturas/" + x[0], name: x[1], tone: x[2] } })
    }
    RowLayout {
        Layout.fillWidth: true
        spacing: 18
        Rectangle {
            id: pv
            Layout.preferredWidth: 400; Layout.preferredHeight: 250
            radius: 14; color: Ui.pal.bg; clip: true
            border.color: Ui.line
            Image {
                anchors { fill: parent; margins: 1 }
                source: p.hoverWall !== "" ? p.hoverWall : p.curWall
                fillMode: Image.PreserveAspectCrop; asynchronous: true; sourceSize: Qt.size(800, 500)
                Behavior on source { enabled: false }
            }
            // ventana de ejemplo
            Rectangle {
                x: 70; y: 44; width: 200; height: 120; radius: 7
                color: Ui.dark ? "#ee1d0b12" : "#f4fbf7f2"; border.color: "#30ffffff"
                Row { anchors { right: parent.right; top: parent.top; margins: 8 } spacing: 5
                      Repeater { model: 3; Rectangle { width: 7; height: 7; radius: 3.5; color: index === 2 ? "#e5484d" : Ui.muted; opacity: 0.8 } } }
                Column { x: 12; y: 26; spacing: 7
                         Repeater { model: [150, 120, 160, 90]; Rectangle { width: modelData; height: 6; radius: 3; color: Ui.muted; opacity: 0.35 } } }
            }
            // barra de tareas en pequeño, como la de verdad (translúcida y del color del tema)
            Rectangle {
                anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: 1 }
                height: 24; color: Ui.dark ? "#cc1a0a10" : "#d8fbf6f1"
                Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: "#26ffffff" }
                Rectangle { x: 8; anchors.verticalCenter: parent.verticalCenter; width: 15; height: 15; radius: 7.5; color: Ui.accent
                            Image { anchors.centerIn: parent; source: "qrc:/icons/app-rros.png"; sourceSize: Qt.size(13, 13) } }
                Row { anchors.centerIn: parent; spacing: 5
                      Repeater { model: ["#3b82f6", "#f97316", Ui.accent, "#22c55e", "#8b5cf6"]; Rectangle { width: 13; height: 13; radius: 4; color: modelData } } }
                Txt { anchors { right: parent.right; rightMargin: 10; verticalCenter: parent.verticalCenter } text: Qt.formatTime(new Date(), "HH:mm"); font.pixelSize: 9 }
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6
            Txt { text: p.hoverWall !== "" ? "Vista previa" : "Tu escritorio"; font.weight: Font.DemiBold; font.pixelSize: Ui.fs + 2 }
            Txt { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: Ui.muted
                  text: "Pasa el ratón por un fondo para verlo aquí y haz clic para ponerlo. La barra de tareas toma el color del tema." }
        }
    }
    Section { text: "Fondos de Lily" }
    Flow {
        Layout.fillWidth: true
        spacing: 12
        Repeater {
            model: p.lilyWalls
            ColumnLayout {
                spacing: 4
                Rectangle {
                    property bool on: Ui.v("ui.wallpaper") === modelData.path
                    width: 168; height: 105; radius: 10; color: Ui.card; clip: true
                    border.width: on ? 3 : 1; border.color: on ? Ui.accent : (wm.containsMouse ? Ui.muted : Ui.line)
                    Image { anchors { fill: parent; margins: parent.on ? 3 : 1 } source: "file://" + modelData.thumb; fillMode: Image.PreserveAspectCrop; asynchronous: true }
                    Rectangle { visible: parent.on; anchors { right: parent.right; top: parent.top; margins: 8 } width: 20; height: 20; radius: 10; color: Ui.accent
                                Txt { anchors.centerIn: parent; text: "✓"; color: Ui.accentText; font.pixelSize: 12; font.weight: Font.Bold } }
                    MouseArea {
                        id: wm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onContainsMouseChanged: p.hoverWall = containsMouse ? "file://" + modelData.path : ""
                        onClicked: Ui.setKey("ui.wallpaper", modelData.path)
                    }
                }
                Txt { text: modelData.name; font.pixelSize: Ui.fs - 2; color: Ui.muted; Layout.alignment: Qt.AlignHCenter }
            }
        }
    }
    Card {
        SRow {
            title: "Fondo generado con los colores del tema"
            subtitle: "Cambia solo cuando cambias de tema o de acento"
            separator: false
        }
        Flow {
            Layout.fillWidth: true
            Layout.margins: 14
            Layout.topMargin: 0
            spacing: 10
            Repeater {
                model: [["aurora", "Aurora"], ["ondas", "Ondas"], ["degradado", "Degradado"], ["circulos", "Círculos"], ["petalos", "Pétalos"], ["dunas", "Dunas"], ["malla", "Malla"], ["cristal", "Cristal"], ["topografico", "Topográfico"], ["seda", "Seda"], ["liso", "Liso"]]
                ColumnLayout {
                    spacing: 4
                    Rectangle {
                        property bool on: Ui.v("ui.wallpaper") === "" && Ui.v("ui.wallpaper_style") === modelData[0]
                        width: 120; height: 68; radius: 8
                        color: Ui.pal.bg
                        border.width: on ? 3 : 1
                        border.color: on ? Ui.accent : Ui.line
                        clip: true
                        Image { anchors.fill: parent; anchors.margins: 2; source: p.stylePreview(modelData[0]); fillMode: Image.PreserveAspectCrop; asynchronous: true }
                        MouseArea {
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor; hoverEnabled: true
                            onContainsMouseChanged: p.hoverWall = containsMouse ? p.stylePreview(modelData[0]) : ""
                            onClicked: {
                                var st = modelData[0]
                                // primero se quita «Tu imagen» (taparía el fondo generado) y después se elige el estilo
                                if (Ui.v("ui.wallpaper") !== "") Ui.unsetKey("ui.wallpaper", function () { Ui.setKey("ui.wallpaper_style", st) })
                                else Ui.setKey("ui.wallpaper_style", st)
                            }
                        }
                    }
                    Txt { text: modelData[1]; font.pixelSize: Ui.fs - 2; color: Ui.muted; Layout.alignment: Qt.AlignHCenter }
                }
            }
        }
        SRow {
            title: "Tu imagen"
            subtitle: Ui.v("ui.wallpaper") !== "" ? Ui.v("ui.wallpaper") : "Una foto o imagen tuya (PNG o JPG)"
            separator: false
            RowLayout {
                Btn { text: "Quitar"; visible: Ui.v("ui.wallpaper") !== ""; onClicked: Ui.unsetKey("ui.wallpaper") }
                Btn { text: "Elegir…"; onClicked: pick.open() }
            }
        }
    }
    Section { text: "Temas completos" }
    Flow {
        Layout.fillWidth: true
        spacing: 12
        Repeater {
            model: p.packs
            Rectangle {
                // lo que trae el tema, leído de su archivo: fondo, claro u oscuro, acento y cursor
                readonly property var kv: {
                    var o = {}
                    Sys.readFile("/usr/share/rros/temas/" + modelData.id + ".lilytema").split("\n").forEach(function (l) {
                        var m = l.match(/^\s*([\w.]+)\s*=\s*"?(.*?)"?\s*$/); if (m) o[m[1]] = m[2] })
                    return o
                }
                readonly property bool darkPack: kv["ui.color_scheme"] !== "light"
                width: 220; height: 150; radius: 12; clip: true
                color: Ui.card
                border.width: pm.containsMouse ? 2 : 1; border.color: pm.containsMouse ? Ui.accent : Ui.line
                Image {
                    x: 6; y: 6; width: parent.width - 12; height: 92
                    fillMode: Image.PreserveAspectCrop; asynchronous: true
                    source: kv["ui.wallpaper"] ? "file://" + kv["ui.wallpaper"] : p.stylePreview(kv["ui.wallpaper_style"] || "aurora")
                    // ventana de muestra con los colores del tema
                    Rectangle {
                        x: 18; y: 14; width: 100; height: 56; radius: 5
                        color: parent.parent.darkPack ? "#ee1d0b12" : "#f4fbf7f2"
                        Rectangle { x: 8; y: 10; width: 60; height: 5; radius: 2.5; color: parent.parent.parent.darkPack ? "#80ffffff" : "#40000000" }
                        Rectangle { x: 8; y: 22; width: 44; height: 5; radius: 2.5; color: parent.parent.parent.darkPack ? "#50ffffff" : "#25000000" }
                        Rectangle { x: 8; y: 38; width: 34; height: 10; radius: 5; color: parent.parent.parent.kv["ui.accent"] || Ui.accent }
                    }
                    Rectangle { anchors { left: parent.left; right: parent.right; bottom: parent.bottom } height: 12
                                color: parent.parent.darkPack ? "#cc1a0a10" : "#d8fbf6f1" }
                }
                Txt { x: 12; y: 106; text: modelData.name; font.weight: Font.DemiBold }
                Txt { x: 12; y: 126; text: (darkPack ? "Oscuro" : "Claro") + (kv["ui.cursor_theme"] ? " · " + kv["ui.cursor_theme"].replace("-", " ") : "")
                      color: Ui.muted; font.pixelSize: Ui.fs - 2 }
                MouseArea { id: pm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            onClicked: Sys.run(["rros-tema", "aplicar", modelData.id], function (r) { Ui.toast(r.code === 0 ? "Tema «" + modelData.name + "» aplicado" : "No se pudo", r.code !== 0); Ui.reload() }) }
            }
        }
    }
    Card {
        SRow {
            title: "Compartir tu tema"
            subtitle: "Colores, acento, fondo, escena animada, puntero y letra en un archivo .lilytema"
            separator: false
            RowLayout {
                Btn { text: "Importar…"; onClicked: themeIn.open() }
                Btn { text: "Exportar…"; onClicked: themeOut.open() }
            }
        }
    }
    FileDialog { id: themeIn; title: "Tema de Lily"; nameFilters: ["Temas de Lily (*.lilytema)"]
                 onAccepted: Sys.run(["rros-tema", "importar", decodeURIComponent(String(selectedFile).replace("file://", ""))], function (r) { Ui.toast(r.out.trim() || r.err, r.code !== 0); Ui.reload() }) }
    FileDialog { id: themeOut; title: "Guardar el tema"; fileMode: FileDialog.SaveFile; nameFilters: ["Temas de Lily (*.lilytema)"]
                 onAccepted: { var f = decodeURIComponent(String(selectedFile).replace("file://", "")); if (!/\.lilytema$/.test(f)) f += ".lilytema"
                               Sys.run(["rros-tema", "exportar", f, "Mi tema"], function (r) { Ui.toast(r.out.trim() || r.err, r.code !== 0) }) } }
    property var packs: []
    property var videos: []
    function loadExtras() {
        Sys.run(["rros-tema", "lista"], function (r) { p.packs = r.out.split("\n").filter(function (l) { return l !== "" }).map(function (l) { var x = l.split("|"); return { id: x[0], name: x[1] } }) })
        p.videos = Sys.listDir(Sys.env("HOME") + "/Vídeos/Fondos", ["*.mp4", "*.webm", "*.mkv", "*.gif"])
    }

    Section { text: "Fondo animado" }
    Card {
        SRow {
            title: "Escena animada"
            subtitle: "Encima del fondo, con muy poco gasto. Funciona también en máquinas virtuales sin aceleración gráfica (con menos partículas)."
        }
        // las escenas, en fichas: un emoji que la resume y su nombre
        Flow {
            Layout.fillWidth: true
            Layout.margins: 14
            Layout.topMargin: 0
            spacing: 8
            Repeater {
                model: [["off", "Ninguna", "∅"], ["petalos", "Pétalos de lirio", "🌺"], ["sakura", "Sakura", "🌸"], ["hojas", "Hojas de otoño", "🍂"],
                        ["nieve", "Nieve", "❄"], ["luciernagas", "Luciérnagas", "✨"], ["burbujas", "Burbujas de luz", "🫧"],
                        ["estrellas", "Estrellas fugaces", "🌠"], ["aurora", "Aurora boreal", "🌌"], ["lluvia", "Lluvia", "🌧"]]
                Rectangle {
                    property bool on: (Ui.v("ui.wallpaper_live") || "off") === modelData[0]
                    implicitWidth: sr.implicitWidth + 24; implicitHeight: 34; radius: 17
                    color: on ? Ui.accent : (sm.containsMouse ? Ui.cardHover : Ui.field)
                    border.color: on ? "transparent" : Ui.line
                    Row { id: sr; anchors.centerIn: parent; spacing: 6
                          Txt { text: modelData[2]; color: parent.parent.on ? Ui.accentText : Ui.textColor }
                          Txt { text: modelData[1]; color: parent.parent.on ? Ui.accentText : Ui.textColor; font.pixelSize: Ui.fs - 1 } }
                    MouseArea { id: sm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                onClicked: Ui.setKey("ui.wallpaper_live", modelData[0]) }
                }
            }
        }
        SRow {
            title: "Vídeo o GIF de fondo"
            subtitle: Ui.v("ui.wallpaper_video") !== "" ? Ui.v("ui.wallpaper_video")
                    : "Un vídeo (mp4, webm, mkv) o un GIF: un fondo de anime, un paisaje… En bucle y sin sonido."
            RowLayout {
                Btn { text: "Quitar"; visible: Ui.v("ui.wallpaper_video") !== ""; onClicked: Ui.setKey("ui.wallpaper_video", "") }
                Btn { text: "Elegir…"; onClicked: pickVideo.open() }
            }
        }
        KeyRow { k: "ui.wallpaper_pause_battery"; title: "Pararlo cuando el portátil va con batería" }
        SRow {
            title: "Galería"
            subtitle: "Tus fondos animados están en Vídeos/Fondos. Descarga más (anime, paisajes, píxel art…) y guárdalos ahí."
            separator: false
            RowLayout {
                Btn { text: "Descargar más"; onClicked: Sys.detached(["xdg-open", "https://motionbgs.com/"]) }
                Btn { text: "Abrir la carpeta"; onClicked: { Sys.writeFile(Sys.env("HOME") + "/Vídeos/Fondos/.lily", ""); Sys.detached(["xdg-open", Sys.env("HOME") + "/Vídeos/Fondos"]) } }
            }
        }
    }
    Flow {
        Layout.fillWidth: true
        spacing: 12
        visible: p.videos.length > 0
        Repeater {
            model: p.videos
            Rectangle {
                property bool on: Ui.v("ui.wallpaper_video") === modelData.path
                width: 180; height: 112; radius: 12; color: Ui.card; clip: true
                border.width: on ? 3 : 1; border.color: on ? Ui.accent : Ui.line
                AnimatedImage { anchors.fill: parent; anchors.margins: 3; visible: /\.gif$/i.test(modelData.path); source: visible ? "file://" + modelData.path : ""; fillMode: Image.PreserveAspectCrop; playing: vm.containsMouse }
                Txt { anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: 8 } text: modelData.name; elide: Text.ElideRight; font.pixelSize: Ui.fs - 2
                      style: Text.Outline; styleColor: "#80000000" }
                MouseArea { id: vm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: Ui.setKey("ui.wallpaper_video", modelData.path) }
            }
        }
    }
    FileDialog {
        id: pickVideo
        title: "Elige un vídeo o un GIF para el fondo"
        nameFilters: ["Vídeos y GIF (*.mp4 *.webm *.mkv *.mov *.gif)"]
        onAccepted: Ui.setKey("ui.wallpaper_video", decodeURIComponent(String(selectedFile).replace("file://", "")))
    }
    FileDialog {
        id: pick
        title: "Elige una imagen de fondo"
        nameFilters: ["Imágenes (*.png *.jpg *.jpeg *.webp)"]
        onAccepted: Ui.setKey("ui.wallpaper", decodeURIComponent(String(selectedFile).replace("file://", "")))
    }

    // letras instaladas (las de símbolos y emoji fuera) y temas de puntero (los que tienen carpeta «cursors»)
    property var fonts: Qt.fontFamilies().filter(function (f) { return !/emoji|symbol|unifont|opensymbol|^dejavu math|^noto (sans|serif) (?!mono)[a-z]+ ?[a-z]*$/i.test(f) || /^noto (sans|serif)$/i.test(f) })
    property var monoFonts: Qt.fontFamilies().filter(function (f) { return /mono|code|courier|consol|hack|fira code|jetbrains/i.test(f) })
    property var cursors: []
    Component.onCompleted: { loadExtras(); cursorsLoad(); loadWalls() }
    function cursorsLoad() { Sys.run(["sh", "-c", "for d in /usr/share/icons/*/cursors \"$HOME\"/.local/share/icons/*/cursors; do [ -d \"$d\" ] && t=${d%/cursors} && n=$(sed -n 's/^Name=//p' \"$t/index.theme\" 2>/dev/null | head -1) && p=$(ls \"$t\"/preview*.png 2>/dev/null | head -1) && echo \"${t##*/}|${n:-${t##*/}}|$p\"; done"], function (r) {
        p.cursors = r.out.split("\n").filter(function (l) { return l !== "" }).map(function (l) { var x = l.split("|"); return { id: x[0], name: x[1], preview: x[2] || "" } })
                             .filter(function (c) { return c.preview !== "" })   // solo los que se pueden ver antes de elegirlos
                             .sort(function (a, b) { return a.name.localeCompare(b.name) })
    }) }

    Section { text: "Letra" }
    Card {
        SRow {
            title: "Letra de la interfaz"
            subtitle: "Menús, ventanas, barra de tareas y aplicaciones"
            Combo {
                implicitWidth: 260
                options: p.fonts.map(function (f) { return [f, f] })
                value: Ui.v("ui.font")
                onPicked: function (v) { Ui.setKey("ui.font", v) }
            }
        }
        KeyRow { k: "ui.font_size"; title: "Tamaño de la letra"; unit: " pt" }
        SRow {
            title: "Letra de la terminal y el código"
            Combo {
                implicitWidth: 260
                options: p.monoFonts.map(function (f) { return [f, f] })
                value: Ui.v("ui.font_mono")
                onPicked: function (v) { Ui.setKey("ui.font_mono", v) }
            }
        }
        SRow {
            title: "Vista previa"
            separator: false
            Txt { text: "El veloz murciélago hindú comía feliz cardillo y kiwi. 0123456789"; font.family: Ui.v("ui.font"); font.pixelSize: 15 }
        }
    }

    Section { text: "Puntero del ratón" }
    Flow {
        Layout.fillWidth: true
        spacing: 12
        Repeater {
            model: p.cursors
            Rectangle {
                property bool on: Ui.v("ui.cursor_theme") === modelData.id
                width: 220; height: 116; radius: 12
                color: cm.containsMouse ? Ui.cardHover : Ui.card
                border.width: on ? 3 : 1
                border.color: on ? Ui.accent : Ui.line
                // muestra con la flecha, la mano y el de espera, sobre un fondo neutro para verlos bien
                Rectangle {
                    anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; topMargin: 10 }
                    width: 196; height: 62; radius: 8; color: Ui.dark ? "#2a1a1f" : "#ece5df"
                }
                Image {
                    anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; topMargin: 16 }
                    width: 180; height: 50; fillMode: Image.PreserveAspectFit; smooth: true
                    source: modelData.preview !== "" ? "file://" + modelData.preview : ""
                    visible: modelData.preview !== ""
                }
                Txt { anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; topMargin: 32 }
                      visible: modelData.preview === ""; text: "Sin muestra"; color: Ui.muted; font.pixelSize: Ui.fs - 2 }
                Txt { anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 12 }
                      text: modelData.name; font.weight: Font.DemiBold; font.pixelSize: Ui.fs - 1 }
                Rectangle { visible: parent.on; anchors { right: parent.right; top: parent.top; margins: 6 } width: 20; height: 20; radius: 10; color: Ui.accent
                            Txt { anchors.centerIn: parent; text: "✓"; color: Ui.accentText; font.pixelSize: 12; font.weight: Font.Bold } }
                MouseArea { id: cm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            onClicked: { Ui.setKey("ui.cursor_theme", modelData.id); Ui.toast("Puntero «" + modelData.name + "»: ya está puesto en todo el sistema", false) } }
            }
        }
    }
    Card {
        SRow {
            title: "Pruébalo"
            subtitle: "Pasa el ratón por cada cuadro para ver el puntero que se pone en cada caso"
            RowLayout {
                spacing: 6
                Repeater {
                    model: [["Normal", Qt.ArrowCursor], ["Enlace", Qt.PointingHandCursor], ["Texto", Qt.IBeamCursor], ["Mover", Qt.SizeAllCursor], ["Espera", Qt.BusyCursor]]
                    Rectangle {
                        width: 64; height: 40; radius: 8; color: tm.containsMouse ? Ui.cardHover : "transparent"; border.color: Ui.line
                        Txt { anchors.centerIn: parent; text: modelData[0]; font.pixelSize: Ui.fs - 2 }
                        MouseArea { id: tm; anchors.fill: parent; hoverEnabled: true; cursorShape: modelData[1] }
                    }
                }
            }
        }
        KeyRow { k: "ui.cursor_size"; title: "Tamaño del puntero"; unit: " px"; step: 4 }
        KeyRow { k: "ui.transparency"; title: "Transparencia de menús y ventanas sin foco"; unit: " %"; step: 5; separator: false }
    }
}
