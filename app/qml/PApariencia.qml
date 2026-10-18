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
    FileDialog {
        id: pick
        title: "Elige una imagen de fondo"
        nameFilters: ["Imágenes (*.png *.jpg *.jpeg *.webp)"]
        onAccepted: Ui.setKey("ui.wallpaper", decodeURIComponent(String(selectedFile).replace("file://", "")))
    }

    Section { text: "Letra e iconos" }
    Card {
        KeyRow { k: "ui.font"; title: "Letra de la interfaz"; combo: true }
        KeyRow { k: "ui.font_size"; title: "Tamaño de la letra"; unit: " pt" }
        KeyRow { k: "ui.font_mono"; title: "Letra de la terminal y el código" }
        KeyRow { k: "ui.cursor_size"; title: "Tamaño del puntero"; unit: " px"; step: 4 }
        KeyRow { k: "ui.transparency"; title: "Transparencia de menús y ventanas sin foco"; unit: " %"; step: 5; separator: false }
    }
}
