// Centro de control: Wi-Fi, Bluetooth, modo oscuro, no molestar, luz nocturna, brillo, volumen y los avisos
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

Glass {
    id: cc
    strong: true
    width: 340
    height: col.implicitHeight + 28
    radius: 18
    // modos de Lily (rros-modo): enfoque y juego
    property string mode: "normal"
    function setMode(m) { mode = m; Sh.run(["rros-modo", m], function () {}) }
    onVisibleChanged: if (visible) Sh.run(["rros-modo", "estado"], function (r) { cc.mode = r.out.trim() || "normal" })
    ColumnLayout {
        id: col
        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 14 }
        spacing: 10
        GridLayout {
            columns: 2
            columnSpacing: 10; rowSpacing: 10
            Layout.fillWidth: true
            Toggle { title: "Wi-Fi"; sub: Sy.wifiOn ? (Sy.netType === "wifi" ? Sy.netName : "Encendido") : "Apagado"; glyph: "wifi"; on: Sy.wifiOn
                     onToggled: Sy.setWifi(!Sy.wifiOn); onMore: { Sh.detached(["rros-ajustes", "--pagina", "red"]); Ux.close() } }
            Toggle { title: "Bluetooth"; sub: !Sy.btPresent ? "No disponible" : Sy.btOn ? "Encendido" : "Apagado"; glyph: "bluetooth"; on: Sy.btOn
                     onToggled: Sy.setBt(!Sy.btOn); onMore: { Sh.detached(["rros-ajustes", "--pagina", "bluetooth"]); Ux.close() } }
            Toggle { title: "Modo oscuro"; sub: Th.dark ? "Activado" : "Desactivado"; glyph: "moon"; on: Th.dark
                     onToggled: Sh.run(["rros-theme", "toggle"], function () { Th.reload() }) }
            Toggle { title: "No molestar"; sub: Notifs.doNotDisturb ? "Avisos en silencio" : "Desactivado"; glyph: "dnd"; on: Notifs.doNotDisturb
                     onToggled: Notifs.doNotDisturb = !Notifs.doNotDisturb }
            Toggle { title: "Luz nocturna"; sub: Sy.nightLight ? "Activada" : "Desactivada"; glyph: "night"; on: Sy.nightLight
                     onToggled: Th.set("ui.night_light", Sy.nightLight ? "off" : "on") }
            Toggle { title: "Enfoque"; sub: cc.mode === "enfoque" ? "Sin avisos ni animaciones" : "Desactivado"; glyph: "dnd"; on: cc.mode === "enfoque"
                     onToggled: cc.setMode(cc.mode === "enfoque" ? "normal" : "enfoque") }
            Toggle { title: "Modo juego"; sub: cc.mode === "juego" ? "Máximo rendimiento" : "Desactivado"; glyph: "power"; on: cc.mode === "juego"
                     onToggled: cc.setMode(cc.mode === "juego" ? "normal" : "juego") }
            Toggle { title: "Captura"; sub: "De la pantalla"; glyph: "camera"; on: false; toggle: false
                     onToggled: { Ux.close(); Sh.detached(["rros-ajustes", "--app", "capturas"]) } }
            Toggle { title: "Móvil"; sub: "KDE Connect"; glyph: "bluetooth"; on: false; toggle: false
                     onToggled: { Ux.close(); Sh.detached(["rros-ajustes", "--pagina", "movil"]) } }
        }
        SliderRow { visible: Sy.brightness >= 0; title: "Pantalla"; glyph: "sun"; value: Sy.brightness; to: 100; onMoved: function (v) { Sy.setBrightness(Math.max(1, v)) } }
        SliderRow { title: "Sonido"; glyph: "volume"; value: Sy.muted ? 0 : Sy.volume; to: 100; onMoved: function (v) { Sy.setVolume(v) } }
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Round { glyph: "gear"; tip: "Ajustes"; onClicked: { Sh.detached(["rros-ajustes"]); Ux.close() } }
            Round { glyph: "lock"; tip: "Bloquear"; onClicked: { Ux.close(); Sh.detached(["loginctl", "lock-session"]) } }
            Item { Layout.fillWidth: true }
            Round { glyph: "power"; tip: "Apagar"; onClicked: { Ux.close(); Sh.detached(["qdbus6", "org.kde.LogoutPrompt", "/LogoutPrompt", "promptShutDown"]) } }
        }
        // avisos
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 4
            visible: Notifs.count > 0
            Txt { text: "Avisos"; font.weight: Font.Bold; Layout.fillWidth: true }
            Txt { text: "Borrar"; color: Th.accent; MouseArea { anchors.fill: parent; onClicked: Notifs.clear() } }
        }
        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, 260)
            visible: Notifs.count > 0
            clip: true
            spacing: 8
            model: Notifs
            delegate: Rectangle {
                width: ListView.view.width; height: nc.implicitHeight + 18; radius: 12; color: Th.tile
                ColumnLayout {
                    id: nc
                    anchors { left: parent.left; right: parent.right; top: parent.top; margins: 9 }
                    spacing: 2
                    RowLayout { Txt { text: model.app; color: Th.muted; font.pixelSize: 11; Layout.fillWidth: true } Txt { text: model.time; color: Th.muted; font.pixelSize: 11 } }
                    Txt { text: model.summary; font.weight: Font.DemiBold; Layout.fillWidth: true }
                    Txt { text: model.body; color: Th.muted; wrapMode: Text.WordWrap; maximumLineCount: 3; textFormat: Text.PlainText; Layout.fillWidth: true; visible: text !== "" }
                }
                MouseArea { anchors.fill: parent; onClicked: Notifs.dismiss(model.nid) }
            }
        }
    }

    component Toggle: Rectangle {
        id: tg
        property string title
        property string sub
        property string glyph
        property bool on
        property bool toggle: true
        signal toggled()
        signal more()
        Layout.fillWidth: true
        implicitHeight: 58
        radius: 14
        color: Th.tile
        Rectangle {
            id: dot
            x: 10; anchors.verticalCenter: parent.verticalCenter
            width: 34; height: 34; radius: 17
            color: tg.on ? Th.accent : (Th.dark ? Qt.rgba(1, 1, 1, 0.16) : Qt.rgba(0, 0, 0, 0.10))
            Glyph { anchors.centerIn: parent; name: tg.glyph; color: tg.on ? "#ffffff" : Th.text; level: 1 }
            MouseArea { anchors.fill: parent; onClicked: tg.toggled() }
        }
        Column {
            anchors { left: dot.right; leftMargin: 9; right: parent.right; rightMargin: 6; verticalCenter: parent.verticalCenter }
            Txt { text: tg.title; font.weight: Font.DemiBold; font.pixelSize: 12; width: parent.width }
            Txt { text: tg.sub; color: Th.muted; font.pixelSize: 11; width: parent.width }
        }
        MouseArea { anchors.fill: parent; anchors.leftMargin: 46; onClicked: tg.toggle ? tg.more() : tg.toggled() }
    }
    component SliderRow: Column {
        id: sr
        property string title
        property string glyph
        property real value
        property real to: 100
        signal moved(real v)
        Layout.fillWidth: true
        spacing: 6
        Txt { text: sr.title; font.weight: Font.DemiBold; font.pixelSize: 12 }
        Rectangle {
            width: parent.width; height: 26; radius: 13; color: Th.tile
            Rectangle {
                width: Math.max(26, parent.width * Math.min(1, sl.value / sr.to)); height: parent.height; radius: 13
                color: Th.dark ? Qt.rgba(1, 1, 1, 0.85) : "#ffffff"
                border.color: Th.edge
            }
            Glyph { x: 6; anchors.verticalCenter: parent.verticalCenter; name: sr.glyph; color: "#202230"; level: sl.value / sr.to }
            Slider {
                id: sl
                anchors.fill: parent
                from: 0; to: sr.to; value: sr.value
                background: Item {}
                handle: Item {}
                onMoved: sr.moved(value)
            }
        }
    }
    component Round: Rectangle {
        id: rb
        property string glyph
        property string tip
        signal clicked()
        implicitWidth: 38; implicitHeight: 38; radius: 19
        color: rm.containsMouse ? Th.hover : Th.tile
        Glyph { anchors.centerIn: parent; name: rb.glyph }
        MouseArea { id: rm; anchors.fill: parent; hoverEnabled: true; onClicked: rb.clicked() }
    }
}
