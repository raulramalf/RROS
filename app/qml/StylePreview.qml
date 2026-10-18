// Dibujo de un estilo de escritorio: moderno, clásico o enfoque
import QtQuick
import "."

Rectangle {
    id: sp
    property string kind: "moderno"
    width: 190; height: 112; radius: 10
    clip: true
    gradient: Gradient {
        GradientStop { position: 0; color: Qt.darker(Ui.accent, 2.4) }
        GradientStop { position: 1; color: Qt.darker(Ui.pal.cyan, 3) }
    }
    // ventana
    Rectangle {
        x: 34; y: sp.kind === "clasico" ? 18 : 24; width: 110; height: 58; radius: 5
        color: Ui.dark ? "#26304f" : "#f4f6fb"
        Row { x: 6; y: 5; spacing: 3; Repeater { model: ["#ff5f57", "#febc2e", "#28c840"]; Rectangle { width: 6; height: 6; radius: 3; color: modelData } } }
    }
    // barra de arriba (moderno y enfoque)
    Rectangle {
        visible: sp.kind !== "clasico"
        x: sp.kind === "enfoque" ? 55 : 0; y: sp.kind === "enfoque" ? 4 : 0
        width: sp.kind === "enfoque" ? 80 : parent.width; height: 11; radius: sp.kind === "enfoque" ? 5 : 0
        color: "#55000000"
        Rectangle { x: 5; anchors.verticalCenter: parent.verticalCenter; width: 6; height: 6; radius: 2; color: "#ffffff"; visible: sp.kind === "moderno" }
        Rectangle { anchors.centerIn: parent; width: 22; height: 4; radius: 2; color: "#ddffffff"; visible: sp.kind === "enfoque" }
        Rectangle { anchors { right: parent.right; rightMargin: 6; verticalCenter: parent.verticalCenter } width: 16; height: 4; radius: 2; color: "#bbffffff"; visible: sp.kind === "moderno" }
    }
    // dock
    Rectangle {
        visible: sp.kind !== "clasico"
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: sp.kind === "enfoque" ? -14 : 6 }
        width: 96; height: 18; radius: 6
        color: "#45ffffff"
        border.color: "#60ffffff"
        Row { anchors.centerIn: parent; spacing: 4; Repeater { model: ["#3b82f6", "#f97316", "#8b5cf6", "#22c55e", "#ef4444"]; Rectangle { width: 11; height: 11; radius: 3; color: modelData } } }
    }
    // barra de tareas (clásico)
    Rectangle {
        visible: sp.kind === "clasico"
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: 18; color: "#77000000"
        Rectangle { x: 5; anchors.verticalCenter: parent.verticalCenter; width: 10; height: 10; radius: 3; color: Ui.accent }
        Row { x: 22; anchors.verticalCenter: parent.verticalCenter; spacing: 4; Repeater { model: 3; Rectangle { width: 24; height: 9; radius: 2; color: "#55ffffff" } } }
        Rectangle { anchors { right: parent.right; rightMargin: 6; verticalCenter: parent.verticalCenter } width: 20; height: 5; radius: 2; color: "#bbffffff" }
    }
}
