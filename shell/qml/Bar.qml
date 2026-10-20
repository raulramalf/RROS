// Barra superior de RROS (estilos Dock y Doble): el menú RROS, la aplicación activa, la bandeja y la fecha y hora.
// Pensada como la de un ordenador: fondo sólido del color de las ventanas, iconos de la bandeja agrupados (un clic
// abre los ajustes rápidos), avisos con su contador y el reloj con la fecha completa.
import QtQuick
import QtQuick.Layouts
import "."

Item {
    id: bar
    property var hostWindow: null
    implicitHeight: Th.barH
    property string now: ""
    property string today: ""
    // dónde se abren los menús, en coordenadas de la pantalla
    function anchorX(item) { var p = item.mapToItem(null, 0, 0); return p.x }
    function anchorR(item) { var p = item.mapToItem(null, item.width, 0); return p.x }
    function menuY() { return Th.barTop ? bar.implicitHeight + 4 : -1 }

    Timer {
        interval: 1000; running: true; repeat: true; triggeredOnStart: true
        onTriggered: {
            var d = new Date(), es = Qt.locale("es_ES")
            bar.now = d.toLocaleString(es, Th.clock24 ? "HH:mm" : "h:mm AP")
            bar.today = d.toLocaleString(es, "ddd d 'de' MMMM")
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Th.barColor
        Rectangle { anchors { left: parent.left; right: parent.right } y: Th.barTop ? parent.height - 1 : 0; height: 1; color: Th.edge }
    }

    RowLayout {
        anchors { fill: parent; leftMargin: 6; rightMargin: 6 }
        spacing: 2

        BarButton {
            id: logo
            implicitWidth: 34
            onClicked: Ux.toggle("menu", bar.anchorX(logo), bar.menuY())
            Image { anchors.centerIn: parent; source: Th.dark ? "qrc:/icons/app-rros-mono.svg" : "qrc:/icons/app-rros-mono-dark.svg"; sourceSize: Qt.size(44, 44); width: 20; height: 20 }
        }
        Txt {
            Layout.leftMargin: 6
            text: Tk.active ? (Apps.forWindow(Tk.active.appId).name || Tk.active.name || "Aplicación") : "Escritorio"
            font.weight: Font.DemiBold
            Layout.maximumWidth: 240
        }
        Txt {
            visible: Tk.active !== null && Tk.active.title !== ""
            text: Tk.active ? "— " + Tk.active.title : ""
            color: Th.muted
            Layout.maximumWidth: 420
            Layout.leftMargin: 6
        }
        Item { Layout.fillWidth: true }

        // ---- bandeja: red, sonido y batería juntos (como en Windows 11: un clic, ajustes rápidos) ----
        BarButton {
            id: tray
            implicitWidth: trayRow.implicitWidth + 18
            onClicked: Ux.toggle("control", bar.anchorR(tray), bar.menuY())
            Row {
                id: trayRow
                anchors.centerIn: parent
                spacing: 12
                Glyph { anchors.verticalCenter: parent.verticalCenter; name: Sy.netType === "ethernet" ? "ethernet" : (Sy.netType === "wifi" ? "wifi" : "wifi-off"); level: Sy.wifiSignal / 100 }
                Glyph { anchors.verticalCenter: parent.verticalCenter; name: "volume"; level: Sy.volume / 100; flag: Sy.muted || Sy.volume === 0 }
                Row {
                    visible: Sy.battery >= 0
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 4
                    Glyph { name: "battery"; width: 22; height: 16; level: Sy.battery / 100; flag: Sy.charging; anchors.verticalCenter: parent.verticalCenter }
                    Txt { visible: Th.batteryPct; text: Sy.battery + " %"; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
                }
            }
        }
        BarButton {
            id: bell
            onClicked: Ux.toggle("calendar", bar.anchorR(bell), bar.menuY())
            Glyph { anchors.centerIn: parent; name: "bell" }
            Rectangle {
                visible: Notifs.count > 0
                width: Math.max(16, nb.implicitWidth + 8); height: 16; radius: 8
                color: Th.accent
                anchors { right: parent.right; top: parent.top; rightMargin: -2; topMargin: 0 }
                Txt { id: nb; anchors.centerIn: parent; text: Notifs.count; color: Th.onAccent; font.pixelSize: 10; font.weight: Font.Bold }
            }
        }
        BarButton {
            id: clock
            implicitWidth: clockRow.implicitWidth + 18
            onClicked: Ux.toggle("calendar", bar.anchorR(clock), bar.menuY())
            Row {
                id: clockRow
                anchors.centerIn: parent
                spacing: 10
                Txt { text: bar.today; color: Th.muted; anchors.verticalCenter: parent.verticalCenter }
                Txt { text: bar.now; font.weight: Font.DemiBold; anchors.verticalCenter: parent.verticalCenter }
            }
        }
    }

    component BarButton: Rectangle {
        signal clicked()
        default property alias content: inner.data
        implicitWidth: 32
        implicitHeight: Th.barH - 6
        Layout.alignment: Qt.AlignVCenter
        radius: 5
        color: ma.pressed ? Th.pressed : ma.containsMouse ? Th.hover : "transparent"
        Item { id: inner; anchors.fill: parent }
        MouseArea { id: ma; anchors.fill: parent; hoverEnabled: true; onClicked: parent.clicked() }
    }
}
