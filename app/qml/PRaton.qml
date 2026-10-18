// Ratón y panel táctil: cada aparato por separado (KWin) y los valores por defecto de RROS
import QtQuick
import QtQuick.Layouts
import "."

RPage {
    id: p
    title: "Ratón y panel táctil"
    subtitle: "Velocidad, desplazamiento y clics"
    icon: "qrc:/icons/sec-raton.svg"
    property var devs: []
    function refresh() { devs = Sys.inputDevices() }
    function set(d, prop, v) { if (!Sys.setInputProperty(d.sys, prop, v)) Ui.toast("No se pudo cambiar " + d.name, true); refresh() }
    onShown: refresh()

    Repeater {
        model: p.devs
        ColumnLayout {
            spacing: 10
            Layout.fillWidth: true
            Section { text: (modelData.touchpad ? "Panel táctil · " : "Ratón · ") + modelData.name }
            Card {
                SRow {
                    title: "Velocidad del puntero"
                    Slide { from: -1; to: 1; stepSize: 0.05; value: modelData.pointerAcceleration; onCommitted: function (v) { p.set(modelData, "pointerAcceleration", v) } }
                }
                SRow {
                    title: "Desplazamiento natural"
                    subtitle: "El contenido sigue a los dedos, como en un móvil"
                    Toggle { checked: modelData.naturalScroll; onToggled: function (on) { p.set(modelData, "naturalScroll", on) } }
                }
                SRow {
                    visible: modelData.touchpad
                    title: "Tocar para hacer clic"
                    Toggle { checked: modelData.tapToClick; onToggled: function (on) { p.set(modelData, "tapToClick", on) } }
                }
                SRow {
                    visible: modelData.touchpad
                    title: "Desactivar mientras escribes"
                    Toggle { checked: modelData.disableWhileTyping; onToggled: function (on) { p.set(modelData, "disableWhileTyping", on) } }
                }
                SRow {
                    title: "Para zurdos"
                    subtitle: "Intercambia los botones izquierdo y derecho"
                    separator: false
                    Toggle { checked: modelData.leftHanded; onToggled: function (on) { p.set(modelData, "leftHanded", on) } }
                }
            }
        }
    }
    Note { visible: p.devs.length === 0; text: "No se encontraron ratones ni paneles táctiles (hace falta una sesión de Plasma). Abajo están los valores de RROS para los que conectes." }

    Section { text: "Para todos los aparatos" }
    Card {
        KeyRow { k: "input.pointer_speed"; title: "Velocidad del puntero"; step: 5 }
        KeyRow { k: "input.natural_scroll"; title: "Desplazamiento natural" }
        KeyRow { k: "input.tap_to_click"; title: "Tocar para hacer clic" }
        KeyRow { k: "input.disable_while_typing"; title: "Desactivar el panel táctil al escribir" }
        KeyRow { k: "input.gestures"; title: "Gestos"; subtitle: "Tres dedos: cambiar de escritorio · cuatro: ver todas las ventanas" }
        KeyRow { k: "ui.cursor_size"; title: "Tamaño del puntero"; unit: " px"; step: 4; separator: false }
    }
}
