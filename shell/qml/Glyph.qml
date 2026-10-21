// Símbolos de Lily dibujados a mano (siguen el color del tema): red, sonido, batería, ajustes rápidos...
import QtQuick
import "."

Canvas {
    id: g
    property string name
    property color color: Th.text
    property real level: 1          // 0-1: señal, volumen, batería
    property bool flag: false       // cargando, silenciado...
    width: 16; height: 16
    onNameChanged: requestPaint()
    onColorChanged: requestPaint()
    onLevelChanged: requestPaint()
    onFlagChanged: requestPaint()
    renderTarget: Canvas.Image
    onPaint: {
        var c = getContext("2d"), w = width, h = height, s = Math.min(w, h) / 16
        c.reset()
        c.save()
        c.translate((w - 16 * s) / 2, (h - 16 * s) / 2)
        c.scale(s, s)
        c.strokeStyle = g.color; c.fillStyle = g.color
        c.lineWidth = 1.6; c.lineCap = "round"; c.lineJoin = "round"
        function arc(r, a0, a1) { c.beginPath(); c.arc(8, 13, r, a0, a1); c.stroke() }
        switch (g.name) {
        case "wifi":
            c.globalAlpha = 1
            for (var i = 0; i < 3; i++) {
                c.globalAlpha = g.level > i / 3 + 0.05 ? 1 : 0.3
                arc(3.5 + i * 3.5, Math.PI * 1.25, Math.PI * 1.75)
            }
            c.globalAlpha = 1; c.beginPath(); c.arc(8, 13, 1.3, 0, Math.PI * 2); c.fill()
            break
        case "wifi-off":
            c.globalAlpha = 0.4; arc(7, Math.PI * 1.25, Math.PI * 1.75); arc(10.5, Math.PI * 1.25, Math.PI * 1.75)
            c.globalAlpha = 1; c.beginPath(); c.moveTo(3, 3); c.lineTo(13, 13); c.stroke()
            break
        case "ethernet":
            c.strokeRect(5, 2.5, 6, 5); c.beginPath(); c.moveTo(8, 7.5); c.lineTo(8, 10); c.moveTo(3, 13.5); c.lineTo(3, 10)
            c.lineTo(13, 10); c.lineTo(13, 13.5); c.moveTo(8, 10); c.lineTo(8, 13.5); c.stroke()
            break
        case "volume":
            c.beginPath(); c.moveTo(2, 6); c.lineTo(5, 6); c.lineTo(9, 2.5); c.lineTo(9, 13.5); c.lineTo(5, 10); c.lineTo(2, 10); c.closePath(); c.fill()
            if (g.flag) { c.beginPath(); c.moveTo(11, 6); c.lineTo(15, 10); c.moveTo(15, 6); c.lineTo(11, 10); c.stroke() }
            else {
                if (g.level > 0.05) { c.beginPath(); c.arc(9, 8, 3, -0.9, 0.9); c.stroke() }
                if (g.level > 0.5) { c.beginPath(); c.arc(9, 8, 5.6, -0.9, 0.9); c.stroke() }
            }
            break
        case "battery":
            c.lineWidth = 1.2
            c.beginPath(); c.roundedRect(1, 4.5, 12.5, 7, 2, 2); c.stroke()
            c.fillRect(14.3, 6.5, 1.3, 3)
            c.globalAlpha = g.level <= 0.15 && !g.flag ? 1 : 1
            if (g.level <= 0.15 && !g.flag) c.fillStyle = "#ff5f57"
            c.fillRect(2.6, 6.1, Math.max(0.8, 9.3 * g.level), 3.8)
            if (g.flag) { c.fillStyle = g.color; c.beginPath(); c.moveTo(8.5, 3); c.lineTo(5.5, 8.5); c.lineTo(8, 8.5); c.lineTo(7, 13); c.lineTo(10.5, 7.5); c.lineTo(8, 7.5); c.closePath(); c.fill() }
            break
        case "bluetooth":
            c.beginPath(); c.moveTo(4.5, 5); c.lineTo(11.5, 11); c.lineTo(8, 14); c.lineTo(8, 2); c.lineTo(11.5, 5); c.lineTo(4.5, 11); c.stroke()
            break
        case "control":
            c.beginPath(); c.roundedRect(1.5, 2.5, 13, 5, 2.5, 2.5); c.stroke(); c.beginPath(); c.arc(4.5, 5, 1.6, 0, Math.PI * 2); c.fill()
            c.beginPath(); c.roundedRect(1.5, 9, 13, 5, 2.5, 2.5); c.stroke(); c.beginPath(); c.arc(11.5, 11.5, 1.6, 0, Math.PI * 2); c.fill()
            break
        case "bell":
            c.beginPath(); c.moveTo(3, 11.5); c.lineTo(13, 11.5); c.lineTo(12, 10); c.lineTo(12, 7); c.bezierCurveTo(12, 4, 10, 2.5, 8, 2.5)
            c.bezierCurveTo(6, 2.5, 4, 4, 4, 7); c.lineTo(4, 10); c.closePath(); c.stroke()
            c.beginPath(); c.arc(8, 13, 1.5, 0, Math.PI); c.stroke()
            break
        case "moon":
            c.beginPath(); c.arc(8, 8, 6, Math.PI * 0.35, Math.PI * 1.65, false); c.bezierCurveTo(7, 3.5, 5.5, 9.5, 12.8, 11.4); c.fill()
            break
        case "sun":
            c.beginPath(); c.arc(8, 8, 3, 0, Math.PI * 2); c.fill()
            for (var k = 0; k < 8; k++) { var a = k * Math.PI / 4; c.beginPath(); c.moveTo(8 + Math.cos(a) * 5, 8 + Math.sin(a) * 5); c.lineTo(8 + Math.cos(a) * 6.8, 8 + Math.sin(a) * 6.8); c.stroke() }
            break
        case "dnd":
            c.beginPath(); c.arc(8, 8, 6.2, 0, Math.PI * 2); c.stroke(); c.beginPath(); c.moveTo(4.8, 8); c.lineTo(11.2, 8); c.stroke()
            break
        case "night":
            c.beginPath(); c.arc(8, 8, 6, 0, Math.PI * 2); c.stroke(); c.beginPath(); c.arc(8, 8, 6, Math.PI * 0.5, Math.PI * 1.5); c.fill()
            break
        case "camera":
            c.beginPath(); c.roundedRect(1.5, 4.5, 13, 9, 2, 2); c.stroke(); c.beginPath(); c.arc(8, 9, 2.6, 0, Math.PI * 2); c.stroke()
            c.fillRect(5.5, 2.8, 5, 1.7)
            break
        case "gear":
            c.beginPath(); c.arc(8, 8, 2.4, 0, Math.PI * 2); c.stroke()
            for (var j = 0; j < 8; j++) { var b = j * Math.PI / 4; c.beginPath(); c.moveTo(8 + Math.cos(b) * 4.4, 8 + Math.sin(b) * 4.4); c.lineTo(8 + Math.cos(b) * 6.6, 8 + Math.sin(b) * 6.6); c.stroke() }
            c.beginPath(); c.arc(8, 8, 4.6, 0, Math.PI * 2); c.stroke()
            break
        case "lock":
            c.beginPath(); c.roundedRect(3, 7, 10, 7.5, 1.6, 1.6); c.fill(); c.beginPath(); c.moveTo(5.2, 7); c.lineTo(5.2, 5)
            c.arc(8, 5, 2.8, Math.PI, 0); c.lineTo(10.8, 7); c.stroke()
            break
        case "power":
            c.beginPath(); c.arc(8, 8.8, 5.6, -Math.PI * 0.32, Math.PI * 1.32); c.stroke(); c.beginPath(); c.moveTo(8, 1.8); c.lineTo(8, 8); c.stroke()
            break
        case "search":
            c.beginPath(); c.arc(7, 7, 4.5, 0, Math.PI * 2); c.stroke(); c.beginPath(); c.moveTo(10.4, 10.4); c.lineTo(14, 14); c.stroke()
            break
        case "grid":
            for (var x = 0; x < 3; x++) for (var y = 0; y < 3; y++) { c.beginPath(); c.roundedRect(2 + x * 4.4, 2 + y * 4.4, 3, 3, 0.8, 0.8); c.fill() }
            break
        case "close":
            c.beginPath(); c.moveTo(4, 4); c.lineTo(12, 12); c.moveTo(12, 4); c.lineTo(4, 12); c.stroke()
            break
        case "trash":
            c.beginPath(); c.moveTo(2.5, 4); c.lineTo(13.5, 4); c.moveTo(6, 4); c.lineTo(6.5, 2.2); c.lineTo(9.5, 2.2); c.lineTo(10, 4)
            c.moveTo(3.8, 4); c.lineTo(4.6, 14); c.lineTo(11.4, 14); c.lineTo(12.2, 4); c.stroke()
            break
        }
        c.restore()
    }
}
