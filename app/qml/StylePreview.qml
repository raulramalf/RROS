// Dibujo en miniatura de un estilo de escritorio: rros (Fusión), clasico, isla, moderno (Dock), doble o lateral
import QtQuick
import "."

Rectangle {
    id: sp
    property string kind: "rros"
    readonly property bool hasTop: kind === "moderno" || kind === "doble"
    readonly property bool side: kind === "lateral"
    readonly property bool bottomBar: kind === "rros" || kind === "clasico" || kind === "isla" || kind === "doble"
    readonly property bool centered: kind === "rros" || kind === "isla"
    readonly property color barC: Ui.dark ? "#e0220d13" : "#eefaf5ef"
    readonly property color ink: Ui.dark ? "#ccf6ecdf" : "#cc2a0f16"
    width: 190; height: 112; radius: 10
    clip: true
    gradient: Gradient {
        GradientStop { position: 0; color: Qt.darker(Ui.accent, 2.6) }
        GradientStop { position: 1; color: Qt.darker(Ui.pal.cyan, 2.6) }
    }
    // ventana (con los botones a la derecha, salvo en el estilo Dock)
    Rectangle {
        x: sp.side ? 46 : 38; y: sp.hasTop ? 22 : 14; width: 108; height: 58; radius: 5
        color: Ui.dark ? "#2a1018" : "#fdfaf6"
        Row {
            anchors { top: parent.top; topMargin: 5; right: sp.kind === "moderno" ? undefined : parent.right; rightMargin: 6
                      left: sp.kind === "moderno" ? parent.left : undefined; leftMargin: 6 }
            spacing: 3
            Repeater { model: sp.kind === "moderno" ? ["#ff5f57", "#febc2e", "#28c840"] : [sp.ink, sp.ink, "#e5484d"]
                       Rectangle { width: 6; height: sp.kind === "moderno" ? 6 : 2; radius: 3; color: modelData; anchors.verticalCenter: parent ? undefined : undefined } }
        }
    }
    // barra superior (Dock y Doble)
    Rectangle {
        visible: sp.hasTop
        width: parent.width; height: 11; color: sp.barC
        Rectangle { x: 5; anchors.verticalCenter: parent.verticalCenter; width: 7; height: 7; radius: 2; color: Ui.accent }
        Row { anchors { right: parent.right; rightMargin: 6; verticalCenter: parent.verticalCenter } spacing: 3
              Repeater { model: 3; Rectangle { width: 5; height: 5; radius: 2.5; color: sp.ink } }
              Rectangle { width: 16; height: 4; radius: 2; color: sp.ink } }
    }
    // dock (estilo Dock)
    Rectangle {
        visible: sp.kind === "moderno"
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 6 }
        width: 96; height: 18; radius: 6
        color: "#45ffffff"; border.color: "#60ffffff"
        Row { anchors.centerIn: parent; spacing: 4; Repeater { model: ["#3b82f6", "#f97316", "#c8243c", "#22c55e", "#8b5cf6"]; Rectangle { width: 11; height: 11; radius: 3; color: modelData } } }
    }
    // barra de tareas (abajo de borde a borde, isla o lateral)
    Rectangle {
        visible: sp.bottomBar || sp.side
        x: sp.kind === "isla" ? 14 : 0
        y: sp.side ? 0 : parent.height - height - (sp.kind === "isla" ? 5 : 0)
        width: sp.side ? 20 : parent.width - 2 * x
        height: sp.side ? parent.height : 18
        radius: sp.kind === "isla" ? 7 : 0
        color: sp.barC
        // inicio
        Rectangle { x: sp.side ? 5 : 5; y: sp.side ? 5 : 4; width: 10; height: 10; radius: 3; color: Ui.accent }
        // aplicaciones
        Grid {
            columns: sp.side ? 1 : 6
            spacing: 4
            x: sp.side ? 5 : (sp.centered ? (parent.width - width) / 2 : 22)
            y: sp.side ? 24 : 4
            Repeater {
                model: sp.kind === "clasico" || sp.kind === "doble" ? 3 : 5
                Rectangle {
                    width: sp.kind === "clasico" || sp.kind === "doble" ? 24 : 10; height: 10; radius: 3
                    color: ["#3b82f6", "#f97316", "#c8243c", "#22c55e", "#8b5cf6"][index]
                    opacity: sp.kind === "clasico" || sp.kind === "doble" ? 0.8 : 1
                }
            }
        }
        // bandeja y reloj
        Column {
            visible: sp.kind !== "doble"
            x: sp.side ? 5 : parent.width - 26; y: sp.side ? parent.height - 22 : 4
            spacing: 2
            Rectangle { width: sp.side ? 10 : 20; height: 4; radius: 2; color: sp.ink }
            Rectangle { width: sp.side ? 10 : 14; height: 3; radius: 1.5; color: sp.ink; opacity: 0.6 }
        }
    }
}
