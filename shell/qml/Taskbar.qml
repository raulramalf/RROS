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

    // Windows+T (o el D-Bus focusTaskbar): el foco del teclado pasa al botón de inicio
    function focusFirst() { start.forceActiveFocus() }
    Connections { target: Sh; function onFocusTaskbarRequested() { tb.focusFirst() } }
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
        // brillo suave hacia el escritorio: la barra parece cristal y no una franja pintada
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            gradient: Gradient {
                orientation: tb.v ? Gradient.Horizontal : Gradient.Vertical
                GradientStop { position: 0; color: Qt.rgba(1, 1, 1, Th.dark ? 0.06 : 0.30) }
                GradientStop { position: 1; color: "transparent" }
            }
        }
        Rectangle {   // filo de luz hacia la pantalla
            visible: !Th.island
            color: Th.dark ? Qt.rgba(1, 1, 1, 0.10) : Qt.rgba(0, 0, 0, 0.08)
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
            // solo la foto del lirio, sin fondo ni aro; al pasar el ratón se aclara un poco
            color: "transparent"
            border.width: 0
            Accessible.role: Accessible.Button
            Accessible.name: "Inicio"
            activeFocusOnTab: true
            Keys.onRightPressed: nextItemInFocusChain(true).forceActiveFocus()
            Keys.onDownPressed: nextItemInFocusChain(true).forceActiveFocus()
            Rectangle { anchors.fill: parent; anchors.margins: -2; radius: width / 2; color: "transparent"; border.width: 2; border.color: Th.accent; visible: parent.activeFocus }
            Keys.onReturnPressed: tb.openAt("start", start)
            Keys.onSpacePressed: tb.openAt("start", start)
            Image {
                anchors.centerIn: parent
                source: "qrc:/icons/app-rros.png"
                width: parent.width; height: width
                sourceSize: Qt.size(96, 96)
                smooth: true; mipmap: true
                opacity: sm2.containsMouse || Ux.open === "start" ? 1 : 0.92
                scale: sm2.pressed ? 0.92 : (sm2.containsMouse ? 1.06 : 1)
                Behavior on scale { NumberAnimation { duration: 90 } }
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
        TbButton {
            id: searchBtn
            visible: Th.tbSearch && Th.tbCenter
            Layout.preferredWidth: tb.v ? tb.t - 8 : tb.t - 8
            radius: height / 2
            tip: "Buscar aplicaciones, ajustes y archivos"
            onClicked: tb.openAt("start", start, { search: true })
            Glyph { name: "search"; anchors.centerIn: parent; color: Th.text }
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
                tip: modelData.name + (running ? (focused ? ", activa" : ", abierta") : "")
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
            Layout.preferredWidth: tb.v ? tb.t - 8 : trayFlow.implicitWidth + 22
            Layout.preferredHeight: tb.v ? trayFlow.implicitHeight + 16 : tb.t - 10
            radius: height / 2
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
        // ---- avisos (la campana): abre el centro de avisos, no el calendario ----
        TbButton {
            id: bellBtn
            visible: !Th.topBar
            active: Ux.open === "notifs"
            Layout.preferredWidth: tb.v ? tb.t - 8 : tb.t - 10
            Layout.preferredHeight: tb.v ? tb.t - 8 : tb.t - 10
            radius: height / 2
            onClicked: tb.openAt("notifs", bellBtn)
            tip: "Avisos"
            Glyph { anchors.centerIn: parent; name: "bell" }
            Rectangle {
                visible: Notifs.count > 0
                width: Math.max(15, bn.implicitWidth + 7); height: 15; radius: 7.5; color: Th.accent
                anchors { right: parent.right; top: parent.top; rightMargin: 1; topMargin: 1 }
                Txt { id: bn; anchors.centerIn: parent; text: Notifs.count; color: Th.onAccent; font.pixelSize: 9; font.weight: Font.Bold }
            }
        }
        TbButton {
            id: clock
            visible: !Th.topBar
            active: Ux.open === "calendar"
            Layout.preferredWidth: tb.v ? tb.t - 8 : Math.max(ct.implicitWidth, cd.implicitWidth) + 24
            Layout.preferredHeight: tb.v ? 46 : tb.t - 10
            radius: tb.v ? 7 : height / 2
            onClicked: tb.openAt("calendar", clock)
            tip: "Calendario"
            Column {
                anchors.centerIn: parent
                spacing: 0
                Txt { id: ct; text: tb.now; font.pixelSize: 12; font.weight: Font.DemiBold; anchors.horizontalCenter: parent.horizontalCenter }
                Txt { id: cd; text: tb.date; font.pixelSize: 11; color: Th.muted; anchors.horizontalCenter: parent.horizontalCenter }
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
        // lector de pantalla y teclado: Tab/flechas para moverse, Intro o espacio para abrir, menú contextual con la tecla Menú
        Accessible.role: Accessible.Button
        Accessible.name: tip
        activeFocusOnTab: true
        Keys.onReturnPressed: clicked()
        Keys.onSpacePressed: clicked()
        Keys.onMenuPressed: rightClicked()
        Keys.onRightPressed: nextItemInFocusChain(true).forceActiveFocus()
        Keys.onLeftPressed: nextItemInFocusChain(false).forceActiveFocus()
        Keys.onDownPressed: nextItemInFocusChain(true).forceActiveFocus()
        Keys.onUpPressed: nextItemInFocusChain(false).forceActiveFocus()
        Keys.onEscapePressed: Ux.close()
        Rectangle {   // anillo de foco
            anchors.fill: parent; anchors.margins: -2; radius: parent.radius + 2
            color: "transparent"; border.width: 2; border.color: Th.accent
            visible: parent.activeFocus
        }
        Layout.preferredHeight: tb.v ? tb.t - 8 : tb.t - 8
        Layout.alignment: Qt.AlignCenter
        radius: 9
        color: ma.pressed ? Th.pressed : active ? Th.tile : ma.containsMouse ? Th.hover : "transparent"
        border.width: active ? 1 : 0
        border.color: Th.dark ? Qt.rgba(1, 1, 1, 0.08) : Qt.rgba(0, 0, 0, 0.06)
        Behavior on color { ColorAnimation { duration: 90 } }
        Item { id: inner; anchors.fill: parent }
        MouseArea {
            id: ma; anchors.fill: parent; hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onClicked: function (m) { if (m.button === Qt.RightButton) btn.rightClicked(); else btn.clicked() }
        }
    }
}
