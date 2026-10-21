// Barra de tareas de Lily: botón de inicio, buscador, aplicaciones ancladas y abiertas, bandeja, fecha y hora y,
// en la esquina, «mostrar el escritorio». La estructura es la de Windows; el acabado (iconos que crecen un poco al
// pasar el ratón, indicador de la app activa) recuerda a macOS. Siempre visible: reserva su franja de la pantalla y
// las ventanas maximizadas llegan justo hasta ella. Estilos: Fusión, Clásica, Isla, Doble y Lateral (vertical).
import QtQuick
import QtQuick.Layouts
import "."

Item {
    id: tb
    readonly property bool v: Th.vertical
    readonly property int t: Th.tbH                 // grosor de la barra
    readonly property int icon: Math.round(t * 0.54)
    implicitHeight: v ? 0 : t + (Th.island ? 10 : 0)
    implicitWidth: v ? t : 0
    property string now: ""
    property string date: ""

    Timer {
        interval: 1000; running: true; repeat: true; triggeredOnStart: true
        onTriggered: {
            var d = new Date(), es = Qt.locale("es_ES")
            tb.now = d.toLocaleString(es, Th.clock24 || tb.v ? "HH:mm" : "h:mm AP")
            tb.date = d.toLocaleString(es, tb.v ? "d/M" : "dd/MM/yyyy")
        }
    }

    // dónde abrir los menús (coordenadas de la pantalla): encima de la barra, o a su derecha si es lateral
    function openAt(what, item, d) {
        var p = item.mapToItem(null, 0, 0)
        if (v) Ux.toggle(what, t + 8, Math.max(8, p.y - 40), d)
        else Ux.toggle(what, what === "start" ? p.x : p.x + item.width, -1, d)
    }

    // las aplicaciones: ancladas (en orden) y luego las abiertas que no lo están
    property var items: {
        var out = [], seen = {}
        Th.dockApps.forEach(function (id) {
            var a = Apps.get(id)
            if (!a.id) return
            seen[a.id.toLowerCase()] = 1
            out.push({ id: a.id, name: a.name, icon: a.icon, pinned: true })
        })
        Tk.list.forEach(function (w) {
            var a = Apps.forWindow(w.appId), key = (a.id || w.appId).toLowerCase()
            if (seen[key]) return
            seen[key] = 1
            out.push({ id: a.id || w.appId, name: a.name || w.name, icon: a.icon || w.appId, pinned: false })
        })
        return out
    }

    // ---- fondo: de borde a borde (Fusión, Clásica, Doble, Lateral) o una isla flotante ----
    Rectangle {
        id: plate
        anchors.fill: parent
        anchors.margins: Th.island ? 6 : 0
        anchors.topMargin: Th.island ? 4 : 0
        radius: Th.island ? 14 : 0
        color: Th.barColor
        border.width: Th.island ? 1 : 0
        border.color: Th.edge
        Rectangle {   // filo hacia la pantalla
            visible: !Th.island
            color: Th.edge
            x: tb.v ? parent.width - 1 : 0
            width: tb.v ? 1 : parent.width
            height: tb.v ? parent.height : 1
        }
    }

    GridLayout {
        id: grid
        anchors.fill: plate
        anchors.margins: 4
        flow: tb.v ? GridLayout.TopToBottom : GridLayout.LeftToRight
        rows: tb.v ? -1 : 1
        columns: tb.v ? 1 : -1
        rowSpacing: 2; columnSpacing: 2

        // ---- inicio ----
        // botón de inicio redondo: la flor de Lily en un círculo
        Rectangle {
            id: start
            Layout.preferredWidth: tb.t - 10; Layout.preferredHeight: tb.t - 10
            Layout.alignment: Qt.AlignCenter
            radius: width / 2
            color: sm2.pressed ? Th.pressed : (Ux.open === "start" || sm2.containsMouse) ? Th.hover : "transparent"
            border.width: Ux.open === "start" ? 1 : 0
            border.color: Th.edge
            Accessible.role: Accessible.Button
            Accessible.name: "Inicio"
            activeFocusOnTab: true
            Keys.onReturnPressed: tb.openAt("start", start)
            Keys.onSpacePressed: tb.openAt("start", start)
            Image {
                anchors.centerIn: parent
                source: "qrc:/icons/app-rros.png"
                width: parent.width - 8; height: width
                sourceSize: Qt.size(96, 96)
                smooth: true; mipmap: true
            }
            MouseArea { id: sm2; anchors.fill: parent; hoverEnabled: true; onClicked: tb.openAt("start", start) }
        }
        // ---- buscador ----
        Rectangle {
            id: search
            visible: Th.tbSearch && !Th.tbCenter
            Layout.preferredWidth: 220; Layout.preferredHeight: tb.t - 14
            Layout.alignment: Qt.AlignVCenter
            radius: height / 2
            color: sm.containsMouse ? Th.hover : Th.tile
            border.color: Th.edge
            Row {
                anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
                spacing: 8
                Glyph { name: "search"; color: Th.muted; anchors.verticalCenter: parent.verticalCenter }
                Txt { text: "Buscar"; color: Th.muted; anchors.verticalCenter: parent.verticalCenter }
            }
            MouseArea { id: sm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.IBeamCursor; onClicked: tb.openAt("start", start, { search: true }) }
        }
        Item { visible: Th.tbCenter; Layout.fillWidth: !tb.v; Layout.fillHeight: tb.v }

        // ---- aplicaciones ----
        Repeater {
            model: tb.items
            TbButton {
                id: app
                property var wins: Tk.windowsOf(modelData.id)
                property bool running: wins.length > 0
                property bool focused: Tk.active !== null && wins.some(function (w) { return w.active })
                property string label: Th.tbLabels && running ? (wins[0].title || modelData.name) : ""
                active: focused
                Layout.preferredWidth: tb.v ? tb.t - 8 : (label !== "" ? Math.min(200, tb.t + lbl.implicitWidth + 6) : tb.t - 2)
                onClicked: {
                    if (!running) { Apps.launch(modelData.id); bounce.start(); return }
                    if (focused && wins.length === 1) Tk.toggleMinimized(wins[0].row)
                    else Tk.activate(wins[0].row)
                }
                onRightClicked: {
                    var p = app.mapToItem(null, app.width / 2, 0)
                    Ux.toggle("dockmenu", tb.v ? tb.t + 8 : p.x, tb.v ? p.y + 40 : p.y, { kind: "app", id: modelData.id, pinned: modelData.pinned })
                }
                tip: modelData.name
                Ico {
                    id: ic
                    size: tb.icon
                    name: modelData.icon
                    x: app.label !== "" ? 8 : (parent.width - width) / 2
                    anchors.verticalCenter: parent.verticalCenter
                    scale: app.hovered ? 1.12 : 1.0
                    Behavior on scale { NumberAnimation { duration: Th.anim / 1.5; easing.type: Easing.OutCubic } }
                    SequentialAnimation on anchors.verticalCenterOffset {
                        id: bounce; running: false
                        NumberAnimation { to: -8; duration: 160; easing.type: Easing.OutQuad }
                        NumberAnimation { to: 0; duration: 220; easing.type: Easing.OutBounce }
                    }
                }
                Txt {
                    id: lbl
                    visible: app.label !== ""
                    text: app.label
                    x: ic.x + ic.width + 8
                    width: parent.width - x - 8
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: 12
                }
                // indicador: corto si está abierta, largo y del color del acento si es la ventana activa
                Rectangle {
                    visible: app.running
                    color: app.focused ? Th.accent : Th.muted
                    radius: 1.5
                    width: tb.v ? 3 : (app.focused ? 18 : 6)
                    height: tb.v ? (app.focused ? 18 : 6) : 3
                    x: tb.v ? 1 : (parent.width - width) / 2
                    y: tb.v ? (parent.height - height) / 2 : parent.height - height - 1
                    Behavior on width { NumberAnimation { duration: Th.anim } }
                    Behavior on height { NumberAnimation { duration: Th.anim } }
                }
            }
        }
        Item { Layout.fillWidth: !tb.v; Layout.fillHeight: tb.v }

        // ---- bandeja (en Doble la bandeja está arriba) ----
        TbButton {
            id: tray
            visible: !Th.topBar
            active: Ux.open === "control"
            Layout.preferredWidth: tb.v ? tb.t - 8 : trayFlow.implicitWidth + 20
            Layout.preferredHeight: tb.v ? trayFlow.implicitHeight + 16 : tb.t - 8
            onClicked: tb.openAt("control", tray)
            tip: "Ajustes rápidos"
            Grid {
                id: trayFlow
                anchors.centerIn: parent
                columns: tb.v ? 1 : 4
                spacing: tb.v ? 10 : 12
                Glyph { name: Sy.netType === "ethernet" ? "ethernet" : (Sy.netType === "wifi" ? "wifi" : "wifi-off"); level: Sy.wifiSignal / 100 }
                Glyph { name: "volume"; level: Sy.volume / 100; flag: Sy.muted || Sy.volume === 0 }
                Glyph { visible: Sy.battery >= 0; name: "battery"; width: 22; height: 16; level: Sy.battery / 100; flag: Sy.charging }
            }
        }
        TbButton {
            id: clock
            visible: !Th.topBar
            active: Ux.open === "calendar"
            Layout.preferredWidth: tb.v ? tb.t - 8 : Math.max(ct.implicitWidth, cd.implicitWidth) + 22
            Layout.preferredHeight: tb.v ? 46 : tb.t - 8
            onClicked: tb.openAt("calendar", clock)
            tip: "Calendario y avisos"
            Column {
                anchors.centerIn: parent
                spacing: 0
                Txt { id: ct; text: tb.now; font.pixelSize: 12; font.weight: Font.DemiBold; anchors.horizontalCenter: parent.horizontalCenter }
                Txt { id: cd; text: tb.date; font.pixelSize: 11; color: Th.muted; anchors.horizontalCenter: parent.horizontalCenter }
            }
            Rectangle {
                visible: Notifs.count > 0
                width: 7; height: 7; radius: 3.5; color: Th.accent
                anchors { right: parent.right; top: parent.top; margins: 4 }
            }
        }
        // ---- mostrar el escritorio (la esquina, como en Windows) ----
        Rectangle {
            Layout.preferredWidth: tb.v ? tb.t - 8 : 8
            Layout.preferredHeight: tb.v ? 8 : tb.t - 8
            Layout.alignment: Qt.AlignCenter
            radius: 2
            color: dm.containsMouse ? Th.hover : "transparent"
            Rectangle { color: Th.edge; x: tb.v ? 4 : 0; y: tb.v ? 0 : 4; width: tb.v ? parent.width - 8 : 1; height: tb.v ? 1 : parent.height - 8 }
            MouseArea {
                id: dm; anchors.fill: parent; hoverEnabled: true
                onClicked: Sh.detached(["qdbus6", "org.kde.kglobalaccel", "/component/kwin", "org.kde.kglobalaccel.Component.invokeShortcut", "Show Desktop"])
            }
        }
    }

    // botón de la barra: fondo al pasar el ratón, más marcado si su menú está abierto
    component TbButton: Rectangle {
        id: btn
        signal clicked()
        signal rightClicked()
        default property alias content: inner.data
        property bool active: false
        property bool hovered: ma.containsMouse
        property string tip: ""
        Layout.preferredHeight: tb.v ? tb.t - 8 : tb.t - 8
        Layout.alignment: Qt.AlignCenter
        radius: 7
        color: ma.pressed ? Th.pressed : active ? Th.tile : ma.containsMouse ? Th.hover : "transparent"
        border.width: active ? 1 : 0
        border.color: Th.edge
        Behavior on color { ColorAnimation { duration: 90 } }
        Item { id: inner; anchors.fill: parent }
        MouseArea {
            id: ma; anchors.fill: parent; hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onClicked: function (m) { if (m.button === Qt.RightButton) btn.rightClicked(); else btn.clicked() }
        }
    }
}
