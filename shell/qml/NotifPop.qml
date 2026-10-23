// Centro de avisos (la campana de la barra): los avisos recibidos, «No molestar» y borrar todo
import QtQuick
import QtQuick.Layouts
import "."

Glass {
    id: np
    strong: true
    width: 360
    height: col.implicitHeight + 28
    radius: 18
    ColumnLayout {
        id: col
        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 14 }
        spacing: 10
        RowLayout {
            Layout.fillWidth: true
            Txt { text: "Avisos"; font.pixelSize: 17; font.weight: Font.Bold; Layout.fillWidth: true }
            Rectangle {
                visible: Notifs.count > 0
                implicitWidth: bt.implicitWidth + 20; implicitHeight: 26; radius: 13
                color: bm.containsMouse ? Th.hover : Th.tile
                Txt { id: bt; anchors.centerIn: parent; text: "Borrar todo"; font.pixelSize: 12; color: Th.accent }
                MouseArea { id: bm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: Notifs.clear() }
            }
        }
        // no molestar
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 44; radius: 12; color: Th.tile
            RowLayout {
                anchors { fill: parent; leftMargin: 12; rightMargin: 12 }
                Glyph { name: "dnd" }
                Txt { text: "No molestar"; Layout.fillWidth: true }
                Rectangle {
                    width: 38; height: 22; radius: 11
                    color: Notifs.doNotDisturb ? Th.accent : Th.edge
                    Rectangle { width: 18; height: 18; radius: 9; y: 2; x: Notifs.doNotDisturb ? 18 : 2; color: "white"
                                Behavior on x { NumberAnimation { duration: Th.anim } } }
                }
            }
            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: Notifs.doNotDisturb = !Notifs.doNotDisturb }
        }
        // sin avisos
        Column {
            Layout.fillWidth: true
            Layout.topMargin: 18
            Layout.bottomMargin: 18
            visible: Notifs.count === 0
            spacing: 8
            Glyph { name: "bell"; anchors.horizontalCenter: parent.horizontalCenter; opacity: 0.5 }
            Txt { text: "No tienes avisos"; color: Th.muted; anchors.horizontalCenter: parent.horizontalCenter }
        }
        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, 420)
            visible: Notifs.count > 0
            clip: true
            spacing: 8
            model: Notifs
            delegate: Rectangle {
                width: ListView.view.width; height: nc.implicitHeight + 20; radius: 12
                color: dm.containsMouse ? Th.hover : Th.tile
                ColumnLayout {
                    id: nc
                    anchors { left: parent.left; right: parent.right; top: parent.top; margins: 10; rightMargin: 30 }
                    spacing: 2
                    RowLayout { Txt { text: model.app; color: Th.muted; font.pixelSize: 11; Layout.fillWidth: true } Txt { text: model.time; color: Th.muted; font.pixelSize: 11 } }
                    Txt { text: model.summary; font.weight: Font.DemiBold; Layout.fillWidth: true; elide: Text.ElideRight }
                    Txt { text: model.body; color: Th.muted; wrapMode: Text.WordWrap; maximumLineCount: 4; elide: Text.ElideRight
                          textFormat: Text.PlainText; Layout.fillWidth: true; visible: text !== "" }
                }
                MouseArea { id: dm; anchors.fill: parent; hoverEnabled: true }
                // descartar este aviso
                Rectangle {
                    anchors { right: parent.right; top: parent.top; margins: 8 }
                    width: 20; height: 20; radius: 10
                    visible: dm.containsMouse || xm.containsMouse
                    color: xm.containsMouse ? Th.edge : "transparent"
                    Txt { anchors.centerIn: parent; text: "✕"; font.pixelSize: 11; color: Th.muted }
                    MouseArea { id: xm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: Notifs.dismiss(model.nid) }
                }
            }
        }
    }
}
