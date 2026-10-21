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

    function useTheme(name) {
        Ui.run(["rros-theme", "use", name], function (r) {
            if (r.code !== 0) Ui.toast(r.err || "No se pudo cambiar el tema", true)
            Ui.reload()
        })
    }
    onShown: {
        var st = ["aurora", "ondas", "degradado", "circulos"]
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
                model: [["aurora", "Aurora"], ["ondas", "Ondas"], ["degradado", "Degradado"], ["circulos", "Círculos"], ["liso", "Liso"]]
                ColumnLayout {
                    spacing: 4
                    Rectangle {
                        property bool on: Ui.v("ui.wallpaper") === "" && Ui.v("ui.wallpaper_style") === modelData[0]
                        width: 120; height: 68; radius: 8
                        color: Ui.pal.bg
                        border.width: on ? 3 : 1
                        border.color: on ? Ui.accent : Ui.line
                        clip: true
                        Image { anchors.fill: parent; anchors.margins: 2; source: p.previews[modelData[0]] || ""; fillMode: Image.PreserveAspectCrop; asynchronous: true }
                        MouseArea {
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: { if (Ui.v("ui.wallpaper") !== "") Ui.unsetKey("ui.wallpaper"); Ui.setKey("ui.wallpaper_style", modelData[0]) }
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
                width: 200; height: 64; radius: 12; color: pm.containsMouse ? Ui.cardHover : Ui.card; border.color: Ui.line
                Txt { anchors { left: parent.left; leftMargin: 14; verticalCenter: parent.verticalCenter } text: modelData.name; font.weight: Font.DemiBold }
                MouseArea { id: pm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            onClicked: Sys.run(["rros-tema", "aplicar", modelData.id], function (r) { Ui.toast(r.code === 0 ? "Tema «" + modelData.name + "» aplicado" : "No se pudo", r.code !== 0); Ui.reload() }) }
            }
        }
    }
    Card {
        SRow {
            title: "Compartir tu tema"
            subtitle: "Colores, acento, fondo, escena animada, puntero, letra y diseño de la barra en un archivo .lilytema"
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
            subtitle: "Encima del fondo, con muy poco gasto. Necesita las animaciones activadas."
            Seg { options: [["off", "Ninguna"], ["petalos", "Pétalos"], ["luciernagas", "Luciérnagas"], ["lluvia", "Lluvia"]]
                  value: Ui.v("ui.wallpaper_live"); onPicked: function (v) { Ui.setKey("ui.wallpaper_live", v) } }
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
    Component.onCompleted: { loadExtras(); cursorsLoad() }
    function cursorsLoad() { Sys.run(["sh", "-c", "for d in /usr/share/icons/*/cursors \"$HOME\"/.local/share/icons/*/cursors; do [ -d \"$d\" ] && t=${d%/cursors} && n=$(sed -n 's/^Name=//p' \"$t/index.theme\" 2>/dev/null | head -1) && p=$(ls \"$t\"/preview*.png 2>/dev/null | head -1) && echo \"${t##*/}|${n:-${t##*/}}|$p\"; done"], function (r) {
        p.cursors = r.out.split("\n").filter(function (l) { return l !== "" }).map(function (l) { var x = l.split("|"); return { id: x[0], name: x[1], preview: x[2] || "" } })
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
                width: 150; height: 112; radius: 12
                color: Ui.card
                border.width: on ? 3 : 1
                border.color: on ? Ui.accent : Ui.line
                Image {
                    anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; topMargin: 14 }
                    width: 48; height: 48; fillMode: Image.PreserveAspectFit
                    source: modelData.preview !== "" ? "file://" + modelData.preview : ""
                    visible: modelData.preview !== ""
                }
                Txt { anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 12 }
                      text: modelData.name; font.weight: Font.DemiBold; font.pixelSize: Ui.fs - 1 }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: Ui.setKey("ui.cursor_theme", modelData.id) }
            }
        }
    }
    Card {
        KeyRow { k: "ui.cursor_size"; title: "Tamaño del puntero"; unit: " px"; step: 4 }
        KeyRow { k: "ui.transparency"; title: "Transparencia de menús y ventanas sin foco"; unit: " %"; step: 5; separator: false }
    }
}
