// Launchpad: todas las aplicaciones en pantalla completa, con buscador (escribe directamente)
import QtQuick
import QtQuick.Controls
import "."

Item {
    id: lp
    property string q: ""
    property var results: q === "" ? [] : Apps.search(q, 60)
    function launch(id) { Apps.launch(id); Ux.close() }

    Rectangle { anchors.fill: parent; color: Th.dark ? Qt.rgba(0.02, 0.03, 0.08, 0.78) : Qt.rgba(0.92, 0.93, 0.97, 0.80) }
    MouseArea { anchors.fill: parent; onClicked: Ux.close() }

    TextField {
        id: field
        width: 340; height: 38
        anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; topMargin: parent.height * 0.07 }
        placeholderText: "Buscar"
        color: Th.text
        placeholderTextColor: Th.muted
        font.family: Th.font
        font.pixelSize: 15
        leftPadding: 38
        focus: true
        onTextChanged: lp.q = text
        background: Rectangle { radius: 19; color: Th.tile; border.color: Th.edge
            Glyph { name: "search"; color: Th.muted; x: 13; anchors.verticalCenter: parent.verticalCenter } }
        Keys.onEscapePressed: Ux.close()
        Keys.onReturnPressed: if (lp.results.length) lp.launch(lp.results[0].id)
        Component.onCompleted: forceActiveFocus()
    }
    Connections { target: Ux; function onOpenChanged() { if (Ux.open === "launchpad") { field.text = ""; field.forceActiveFocus() } } }

    GridView {
        id: grid
        readonly property int cols: Math.max(4, Math.min(7, Math.floor((lp.width * 0.8) / 150)))
        width: cols * 150
        anchors { horizontalCenter: parent.horizontalCenter; top: field.bottom; topMargin: 36; bottom: parent.bottom; bottomMargin: 40 }
        cellWidth: 150; cellHeight: 136
        clip: true
        model: lp.q === "" ? Apps : lp.results
        boundsBehavior: Flickable.StopAtBounds
        delegate: Item {
            width: 150; height: 136
            property string aid: lp.q === "" ? model.appId : modelData.id
            property string aname: lp.q === "" ? model.name : modelData.name
            property string aicon: lp.q === "" ? model.iconName : modelData.icon
            Rectangle { anchors.fill: parent; anchors.margins: 6; radius: 16; color: cm.containsMouse ? Th.hover : "transparent" }
            Ico { anchors.horizontalCenter: parent.horizontalCenter; y: 14; size: 72; name: aicon }
            Txt {
                width: parent.width - 16; anchors.horizontalCenter: parent.horizontalCenter; y: 94
                text: aname; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight
                font.pixelSize: 12
            }
            MouseArea { id: cm; anchors.fill: parent; hoverEnabled: true; onClicked: lp.launch(aid) }
        }
    }
    Txt {
        visible: lp.q !== "" && lp.results.length === 0
        anchors.centerIn: parent
        text: "Nada con «" + lp.q + "». La Tienda tiene más aplicaciones."
        color: Th.muted
    }
}
