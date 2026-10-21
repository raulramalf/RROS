// Menú de inicio de Lily: buscador arriba, aplicaciones ancladas, «Todas las aplicaciones» en lista alfabética y,
// abajo, la persona usuaria y los botones de sesión (bloquear, cerrar sesión, reposo, reiniciar, apagar).
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

Glass {
    id: sm
    strong: true
    width: 620; height: 560
    radius: 14
    property string q: ""
    property bool all: false
    property var results: q === "" ? [] : Apps.search(q, 40)
    readonly property string user: Sh.home.split("/").pop()
    function launch(id) { Apps.launch(id); Ux.close() }
    function run(cmd) { Ux.close(); Sh.detached(cmd) }

    Connections {
        target: Ux
        function onOpenChanged() {
            if (Ux.open !== "start") return
            field.text = ""; sm.all = false
            field.forceActiveFocus()
        }
    }

    ColumnLayout {
        anchors { fill: parent; margins: 22; bottomMargin: 0 }
        spacing: 14

        TextField {
            id: field
            Layout.fillWidth: true; Layout.preferredHeight: 38
            placeholderText: "Buscar aplicaciones, ajustes y archivos"
            color: Th.text; placeholderTextColor: Th.muted
            font.family: Th.font; font.pixelSize: 14
            leftPadding: 38
            onTextChanged: sm.q = text
            background: Rectangle {
                radius: 19; color: Th.tile; border.color: field.activeFocus ? Th.accent : Th.edge; border.width: field.activeFocus ? 2 : 1
                Glyph { name: "search"; color: Th.muted; x: 13; anchors.verticalCenter: parent.verticalCenter }
            }
            Keys.onEscapePressed: Ux.close()
            Keys.onReturnPressed: if (sm.results.length) sm.launch(sm.results[0].id)
        }

        // ---- cabecera de la sección ----
        RowLayout {
            Layout.fillWidth: true
            visible: sm.q === ""
            Txt { text: sm.all ? "Todas las aplicaciones" : "Ancladas"; font.weight: Font.DemiBold; font.pixelSize: 14; Layout.fillWidth: true }
            Rectangle {
                implicitWidth: sw.implicitWidth + 20; implicitHeight: 26; radius: 6
                color: swm.containsMouse ? Th.hover : Th.tile
                Txt { id: sw; anchors.centerIn: parent; text: sm.all ? "‹  Atrás" : "Todas  ›"; font.pixelSize: 12 }
                MouseArea { id: swm; anchors.fill: parent; hoverEnabled: true; onClicked: sm.all = !sm.all }
            }
        }

        // ---- ancladas ----
        GridView {
            id: pins
            visible: sm.q === "" && !sm.all
            Layout.fillWidth: true; Layout.fillHeight: true
            cellWidth: Math.floor(width / 6); cellHeight: 92
            clip: true; interactive: false
            model: Th.dockApps.concat(["rros-apps", "org.kde.dolphin", "org.kde.konsole", "org.kde.spectacle", "org.kde.okular", "org.kde.gwenview",
                                       "org.kde.kcalc", "org.kde.ark", "rros-security", "rros-updates", "org.kde.plasma-systemmonitor"])
                           .filter(function (id, i, all) { return all.indexOf(id) === i && Apps.get(id).id }).slice(0, 18)
            delegate: AppCell { app: Apps.get(modelData) }
        }

        // ---- todas, por orden alfabético ----
        ListView {
            id: list
            visible: sm.q === "" && sm.all
            Layout.fillWidth: true; Layout.fillHeight: true
            clip: true; spacing: 2
            model: Apps
            section.property: "name"
            section.criteria: ViewSection.FirstCharacter
            section.delegate: Txt { text: section.toUpperCase(); color: Th.accent; font.weight: Font.Bold; leftPadding: 10; topPadding: 8; bottomPadding: 2 }
            delegate: Row0 { aid: model.appId; aname: model.name; aicon: model.iconName; acomment: model.comment }
            ScrollBar.vertical: ScrollBar {}
        }

        // ---- resultados ----
        ListView {
            visible: sm.q !== ""
            Layout.fillWidth: true; Layout.fillHeight: true
            clip: true; spacing: 2
            model: sm.results
            delegate: Row0 { aid: modelData.id; aname: modelData.name; aicon: modelData.icon; acomment: modelData.comment || ""; first: index === 0 }
            Txt {
                visible: parent.count === 0
                anchors.centerIn: parent
                text: "Nada con «" + sm.q + "».  La Tienda tiene miles de aplicaciones más."
                color: Th.muted
            }
        }

        // ---- pie: persona usuaria y sesión ----
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: -22; Layout.rightMargin: -22
            Layout.preferredHeight: 64
            color: "transparent"
            Rectangle { anchors { left: parent.left; right: parent.right; top: parent.top } height: 1; color: Th.edge }
            RowLayout {
                anchors { fill: parent; leftMargin: 22; rightMargin: 16 }
                spacing: 8
                Rectangle {
                    width: 34; height: 34; radius: 17; color: Th.accent
                    Txt { anchors.centerIn: parent; text: sm.user.charAt(0).toUpperCase(); color: Th.onAccent; font.weight: Font.Bold; font.pixelSize: 15 }
                }
                Txt { text: sm.user; font.weight: Font.DemiBold; Layout.fillWidth: true }
                Repeater {
                    model: [["gear", "Ajustes", ["rros-ajustes"]],
                            ["lock", "Bloquear", ["loginctl", "lock-session"]],
                            ["moon", "Reposo", ["systemctl", "suspend"]],
                            ["power", "Apagar o reiniciar", ["qdbus6", "org.kde.LogoutPrompt", "/LogoutPrompt", "promptShutDown"]]]
                    Rectangle {
                        width: 38; height: 38; radius: 8
                        color: pm.pressed ? Th.pressed : pm.containsMouse ? Th.hover : "transparent"
                        Glyph { anchors.centerIn: parent; name: modelData[0] }
                        MouseArea { id: pm; anchors.fill: parent; hoverEnabled: true; onClicked: sm.run(modelData[2]) }
                        ToolTip.visible: pm.containsMouse; ToolTip.text: modelData[1]; ToolTip.delay: 500
                    }
                }
            }
        }
    }

    component AppCell: Item {
        property var app
        width: pins.cellWidth; height: pins.cellHeight
        Rectangle { anchors.fill: parent; anchors.margins: 3; radius: 8; color: cm.pressed ? Th.pressed : cm.containsMouse ? Th.hover : "transparent" }
        Ico { anchors.horizontalCenter: parent.horizontalCenter; y: 12; size: 40; name: app.icon }
        Txt {
            width: parent.width - 10; anchors.horizontalCenter: parent.horizontalCenter; y: 60
            text: app.name; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight; font.pixelSize: 12
        }
        MouseArea { id: cm; anchors.fill: parent; hoverEnabled: true; onClicked: sm.launch(app.id) }
    }
    component Row0: Rectangle {
        property string aid; property string aname; property string aicon; property string acomment; property bool first: false
        width: ListView.view ? ListView.view.width : 0; height: 48; radius: 8
        color: rm.pressed ? Th.pressed : (rm.containsMouse || first) ? Th.hover : "transparent"
        Ico { x: 10; anchors.verticalCenter: parent.verticalCenter; size: 30; name: parent.aicon }
        Column {
            x: 52; anchors.verticalCenter: parent.verticalCenter; width: parent.width - 60
            Txt { text: aname; width: parent.width; elide: Text.ElideRight }
            Txt { visible: acomment !== ""; text: acomment; color: Th.muted; font.pixelSize: 11; width: parent.width; elide: Text.ElideRight }
        }
        MouseArea { id: rm; anchors.fill: parent; hoverEnabled: true; onClicked: sm.launch(aid) }
    }
}
