// Barra superior de RROS: el menú RROS, la aplicación activa, el estado del sistema, el centro de control y la hora.
// Con «panel.taskbar» (estilo Clásico) es una barra de tareas abajo con las ventanas abiertas.
import QtQuick
import QtQuick.Layouts
import "."

Item {
    id: bar
    property var hostWindow: null
    implicitHeight: Th.barH + (Th.barStyle === "flat" ? 0 : 6)
    property string now: ""
    // dónde se abren los menús, en coordenadas de la pantalla
    function anchorX(item) { var p = item.mapToItem(null, 0, 0); return p.x }
    function menuY() { return Th.barTop ? bar.implicitHeight + 4 : -1 }

    Timer {
        interval: 1000; running: true; repeat: true; triggeredOnStart: true
        onTriggered: bar.now = new Date().toLocaleString(Qt.locale("es_ES"), Th.clock24 ? "ddd d MMM  HH:mm" : "ddd d MMM  h:mm AP")
    }

    Glass {
        id: bg
        anchors.fill: parent
        anchors.topMargin: Th.barStyle === "flat" ? 0 : (Th.barTop ? 6 : 0)
        anchors.bottomMargin: Th.barStyle === "flat" ? 0 : (Th.barTop ? 0 : 6)
        anchors.leftMargin: Th.barStyle === "pill" ? (parent.width - row.implicitWidth - 40) / 2 : Th.barStyle === "floating" ? 8 : 0
        anchors.rightMargin: anchors.leftMargin
        radius: Th.barStyle === "flat" ? 0 : height / 2
        border.width: Th.barStyle === "flat" ? 0 : 1
        Rectangle {   // línea fina abajo (o arriba) en la barra de borde a borde
            visible: Th.barStyle === "flat"
            anchors { left: parent.left; right: parent.right }
            y: Th.barTop ? parent.height - 1 : 0
            height: 1
            color: Th.edge
        }
    }

    RowLayout {
        id: row
        anchors { fill: bg; leftMargin: 10; rightMargin: 12 }
        spacing: 4

        // ---- menú RROS ----
        BarButton {
            id: logo
            onClicked: Ux.toggle("menu", bar.anchorX(logo), bar.menuY())
            Image { anchors.centerIn: parent; source: "qrc:/icons/app-rros.svg"; sourceSize: Qt.size(36, 36); width: 18; height: 18 }
        }
        // ---- la aplicación activa (en negrita, como el nombre de la app en macOS) ----
        Txt {
            visible: !Th.taskbar
            Layout.leftMargin: 6
            text: Tk.active ? (Apps.forWindow(Tk.active.appId).name || Tk.active.name || "Aplicación") : "RROS"
            font.weight: Font.Bold
            Layout.maximumWidth: 260
        }
        Txt {
            visible: !Th.taskbar && Tk.active !== null && Tk.active.title !== ""
            text: Tk.active ? Tk.active.title : ""
            color: Th.muted
            Layout.maximumWidth: 380
            Layout.leftMargin: 10
        }
        // ---- barra de tareas (Clásico) ----
        Repeater {
            model: Th.taskbar ? Tk.list : []
            Rectangle {
                Layout.preferredWidth: Math.min(200, tl.implicitWidth + 44)
                Layout.preferredHeight: Th.barH - 8
                radius: 6
                color: modelData.active ? Qt.rgba(Qt.color(Th.accent).r, Qt.color(Th.accent).g, Qt.color(Th.accent).b, 0.35) : (tm.containsMouse ? Th.hover : "transparent")
                Ico { x: 6; anchors.verticalCenter: parent.verticalCenter; size: 18; name: Apps.forWindow(modelData.appId).icon || modelData.appId }
                Txt { id: tl; x: 30; width: parent.width - 36; anchors.verticalCenter: parent.verticalCenter; text: modelData.title || modelData.name; font.pixelSize: 12 }
                MouseArea { id: tm; anchors.fill: parent; hoverEnabled: true; onClicked: modelData.active ? Tk.toggleMinimized(modelData.row) : Tk.activate(modelData.row) }
            }
        }
        Item { Layout.fillWidth: true }

        // ---- estado ----
        BarButton {
            visible: Sy.battery >= 0
            implicitWidth: bat.width + (Th.batteryPct ? pct.implicitWidth + 6 : 0) + 12
            onClicked: Sh.detached(["rros-ajustes", "--pagina", "energia"])
            Row {
                anchors.centerIn: parent
                spacing: 4
                Txt { id: pct; visible: Th.batteryPct; text: Sy.battery + " %"; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
                Glyph { id: bat; name: "battery"; width: 22; height: 16; level: Sy.battery / 100; flag: Sy.charging; anchors.verticalCenter: parent.verticalCenter }
            }
        }
        BarButton {
            id: net
            onClicked: Ux.toggle("control", bar.anchorX(net), bar.menuY())
            Glyph { anchors.centerIn: parent; name: Sy.netType === "ethernet" ? "ethernet" : (Sy.netType === "wifi" ? "wifi" : "wifi-off"); level: Sy.wifiSignal / 100 }
        }
        BarButton {
            id: vol
            onClicked: Ux.toggle("control", bar.anchorX(vol), bar.menuY())
            Glyph { anchors.centerIn: parent; name: "volume"; level: Sy.volume / 100; flag: Sy.muted || Sy.volume === 0 }
        }
        BarButton {
            id: cc
            onClicked: Ux.toggle("control", bar.anchorX(cc), bar.menuY())
            Glyph { anchors.centerIn: parent; name: "control" }
            Rectangle {   // avisos sin leer
                visible: Notifs.count > 0
                width: 7; height: 7; radius: 3.5
                color: Th.accent
                anchors { right: parent.right; top: parent.top; rightMargin: 3; topMargin: 3 }
            }
        }
        BarButton {
            id: clock
            implicitWidth: ct.implicitWidth + 16
            onClicked: Ux.toggle("calendar", bar.anchorX(clock), bar.menuY())
            Txt { id: ct; anchors.centerIn: parent; text: bar.now; font.weight: Font.DemiBold }
        }
    }

    component BarButton: Rectangle {
        signal clicked()
        default property alias content: inner.data
        implicitWidth: 30
        implicitHeight: Th.barH - 6
        Layout.alignment: Qt.AlignVCenter
        radius: 6
        color: ma.containsMouse || ma.pressed ? Th.hover : "transparent"
        Item { id: inner; anchors.fill: parent }
        MouseArea { id: ma; anchors.fill: parent; hoverEnabled: true; onClicked: parent.clicked() }
    }
}
