// Dibujo del dock en 2D (plano) o en 3D (estante con brillo y reflejo)
import QtQuick
import "."

Item {
    id: d
    property bool threeD: false
    property real s: 1
    width: 190 * s; height: 74 * s
    readonly property var cols: ["#3b82f6", "#f97316", "#8b5cf6", "#22c55e", "#ef4444", "#0ea5e9"]

    Rectangle {
        anchors.fill: parent
        radius: 10 * s
        gradient: Gradient {
            GradientStop { position: 0; color: Qt.darker(Ui.accent, 2.6) }
            GradientStop { position: 1; color: Qt.darker(Ui.pal.cyan, 3.2) }
        }
    }
    // 2D: píldora plana translúcida
    Rectangle {
        visible: !d.threeD
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 8 * s }
        width: 150 * s; height: 30 * s; radius: 10 * s
        color: "#40ffffff"
        border.color: "#55ffffff"
    }
    // 3D: estante en perspectiva, con borde brillante y frente
    Canvas {
        visible: d.threeD
        anchors.fill: parent
        onPaint: {
            var c = getContext("2d"), w = width, h = height, k = d.s
            c.reset()
            var top = h - 22 * k, bot = h - 9 * k
            c.beginPath()
            c.moveTo(w / 2 - 66 * k, top); c.lineTo(w / 2 + 66 * k, top)
            c.lineTo(w / 2 + 82 * k, bot); c.lineTo(w / 2 - 82 * k, bot); c.closePath()
            var g = c.createLinearGradient(0, top, 0, bot)
            g.addColorStop(0, "rgba(255,255,255,0.55)"); g.addColorStop(1, "rgba(255,255,255,0.16)")
            c.fillStyle = g; c.fill()
            c.strokeStyle = "rgba(255,255,255,0.85)"; c.lineWidth = 1
            c.beginPath(); c.moveTo(w / 2 - 66 * k, top); c.lineTo(w / 2 + 66 * k, top); c.stroke()
            c.fillStyle = "rgba(255,255,255,0.28)"
            c.fillRect(w / 2 - 82 * k, bot, 164 * k, 3 * k)
        }
        onVisibleChanged: requestPaint()
    }
    Row {
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: (d.threeD ? 15 : 13) * s }
        spacing: 6 * s
        Repeater {
            model: d.cols
            Item {
                width: 20 * d.s; height: 20 * d.s
                Rectangle { anchors.fill: parent; radius: 5 * d.s; color: modelData
                    Rectangle { anchors { left: parent.left; right: parent.right; top: parent.top } height: parent.height / 2; radius: 5 * d.s; color: "#30ffffff" } }
                // reflejo en el estante
                Rectangle {
                    visible: d.threeD
                    y: parent.height + 1 * d.s
                    width: parent.width; height: 7 * d.s; radius: 3 * d.s
                    gradient: Gradient { GradientStop { position: 0; color: Qt.rgba(Qt.color(modelData).r, Qt.color(modelData).g, Qt.color(modelData).b, 0.45) } GradientStop { position: 1; color: "transparent" } }
                }
            }
        }
    }
}
