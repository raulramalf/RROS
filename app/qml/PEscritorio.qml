// Barra de tareas y escritorio: el diseño de la barra (seis estilos), sus opciones, el dock, la barra superior y las ventanas
import QtQuick
import QtQuick.Layouts
import "."

RPage {
    id: p
    title: "Barra de tareas y escritorio"
    subtitle: "El diseño de la barra de tareas, el dock y las ventanas"
    icon: "qrc:/icons/sec-escritorio.svg"

    function desktop(id) {
        Ui.api("POST", "/api/desktop", { name: id }, function (r) {
            if (!r.ok) Ui.toast(r.error || r.output || "No se pudo cambiar", true)
            Ui.reload()
        })
    }

    Section { text: "Diseño" }
    Flow {
        Layout.fillWidth: true
        spacing: 14
        Repeater {
            model: Ui.st ? Ui.st.desktops : []
            ColumnLayout {
                spacing: 6
                Rectangle {
                    property bool on: Ui.st.desktop_current === modelData.id
                    implicitWidth: 202; implicitHeight: 124; radius: 14
                    color: "transparent"
                    border.width: on ? 3 : 1
                    border.color: on ? Ui.accent : Ui.line
                    StylePreview { anchors.centerIn: parent; kind: modelData.id }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: p.desktop(modelData.id) }
                }
                Txt { text: modelData.label; font.weight: Font.DemiBold; Layout.leftMargin: 4 }
                Txt { text: modelData.desc; color: Ui.muted; font.pixelSize: Ui.fs - 2; wrapMode: Text.WordWrap; elide: Text.ElideNone; Layout.preferredWidth: 202; Layout.leftMargin: 4 }
                Layout.alignment: Qt.AlignTop
            }
        }
    }

    Section { text: "Barra de tareas" }
    Card {
        KeyRow { k: "ui.taskbar_style"; title: "Estilo"; combo: true
                 labels: { "rros": "Fusión", "clasica": "Clásica", "isla": "Isla", "dock": "Dock", "doble": "Doble", "lateral": "Lateral" } }
        KeyRow { k: "ui.taskbar_align"; title: "Aplicaciones"; labels: { "auto": "Según el estilo", "centro": "En el centro", "izquierda": "A la izquierda" }
                 enabled2: Ui.v("ui.taskbar_style") !== "dock" }
        KeyRow { k: "ui.taskbar_size"; title: "Alto"; subtitle: "Siempre visible: las ventanas maximizadas llegan justo hasta ella"; unit: " px"; step: 4
                 enabled2: Ui.v("ui.taskbar_style") !== "dock" }
        KeyRow { k: "ui.taskbar_labels"; title: "Nombre de cada ventana junto a su icono"; enabled2: Ui.v("ui.taskbar_style") !== "dock" }
        KeyRow { k: "ui.taskbar_search"; title: "Buscador en la barra"; subtitle: "Con las aplicaciones a la izquierda"; separator: false
                 enabled2: Ui.v("ui.taskbar_style") !== "dock" }
    }

    Section { text: "Dock (estilo Dock)" }
    Card {
        KeyRow { k: "ui.dock"; title: "Mostrar el dock"; subtitle: "Tus aplicaciones, las abiertas, el Launchpad y la papelera" }
        SRow {
            title: "Aspecto del dock"
            subtitle: "Plano (2D) o con relieve, brillo y el reflejo de los iconos (3D)"
            enabled2: Ui.b("ui.dock")
            RowLayout {
                spacing: 12
                Repeater {
                    model: [["2d", "2D"], ["3d", "3D"]]
                    ColumnLayout {
                        spacing: 4
                        Rectangle {
                            property bool on: Ui.v("ui.dock_style") === modelData[0]
                            implicitWidth: 150; implicitHeight: 62; radius: 10
                            color: "transparent"
                            border.width: on ? 3 : 1
                            border.color: on ? Ui.accent : Ui.line
                            DockPreview { anchors.centerIn: parent; s: 0.74; threeD: modelData[0] === "3d" }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: Ui.setKey("ui.dock_style", modelData[0]) }
                        }
                        Txt { text: modelData[1]; Layout.alignment: Qt.AlignHCenter; font.weight: Font.DemiBold }
                    }
                }
            }
        }
        KeyRow { k: "ui.dock_icon_size"; title: "Tamaño de los iconos"; unit: " px"; step: 4; enabled2: Ui.b("ui.dock") }
        KeyRow { k: "ui.dock_position"; title: "Posición"; labels: { "bottom": "Abajo", "left": "Izquierda", "right": "Derecha" }; enabled2: Ui.b("ui.dock") }
        KeyRow { k: "ui.dock_hiding"; title: "Ocultar"; subtitle: "Siempre visible, se aparta cuando una ventana lo tapa, o aparece al llevar el ratón al borde"
                 labels: { "never": "Nunca", "dodge": "Si estorba", "auto": "Siempre" }; enabled2: Ui.b("ui.dock") }
        KeyRow { k: "ui.dock_launchpad"; title: "Launchpad"; subtitle: "Botón con todas las aplicaciones en pantalla completa"; separator: false; enabled2: Ui.b("ui.dock") }
    }

    Section { text: "Barra superior (estilos Dock y Doble)" }
    Card {
        KeyRow { k: "panel.style"; title: "Forma"; labels: { "flat": "De borde a borde", "floating": "Flotante", "pill": "Píldora" } }
        KeyRow { k: "ui.panel_position"; title: "Posición"; labels: { "top": "Arriba", "bottom": "Abajo", "left": "Izq.", "right": "Der." } }
        KeyRow { k: "ui.panel_size"; title: "Altura"; unit: " px"; step: 2 }
        KeyRow { k: "ui.clock_24h"; title: "Reloj de 24 horas" }
        KeyRow { k: "ui.show_battery_percent"; title: "Porcentaje de batería"; separator: false }
    }

    Section { text: "Ventanas" }
    Card {
        KeyRow { k: "ui.window_buttons"; title: "Botones de las ventanas"; subtitle: "Cerrar, minimizar y maximizar"; labels: { "left": "Izquierda", "right": "Derecha" } }
        KeyRow { k: "ui.effects"; title: "Efectos"; subtitle: "Sin efectos va más rápido en equipos modestos y máquinas virtuales"
                 labels: { "off": "Ninguno", "basic": "Básicos", "advanced": "Todos" } }
        KeyRow { k: "ui.animations"; title: "Animaciones" }
        KeyRow { k: "ui.animation_speed"; title: "Velocidad de las animaciones"; unit: " %"; step: 25 }
        KeyRow { k: "ui.animation_style"; title: "Estilo de las animaciones"; labels: { "suave": "Suave", "rapida": "Rápida", "rebote": "Lámpara", "deslizar": "Deslizar" } }
        KeyRow { k: "ui.blur"; title: "Desenfoque detrás de menús y paneles"; subtitle: "Solo con efectos «Todos»" }
        KeyRow { k: "ui.blur_strength"; title: "Intensidad del desenfoque" }
        KeyRow { k: "ui.dim_inactive"; title: "Oscurecer las ventanas sin foco" }
        KeyRow { k: "ui.hot_corners"; title: "Esquina activa"; subtitle: "Llevar el ratón arriba a la izquierda muestra todas las ventanas" }
        KeyRow { k: "ui.workspaces"; title: "Escritorios virtuales"; separator: false }
    }
}
