// Un icono del dock: crece con el ratón, rebota al abrir, punto si está abierta y nombre encima
import QtQuick
import "."

Item {
    id: di
    property var item
    property var host
    readonly property bool sep: item.kind === "sep"
    readonly property var wins: item.kind === "app" ? Tk.windowsOf(item.id) : []
    readonly property bool running: wins.length > 0
    property int index: 0
    readonly property real center: host.restCenter(index)
    property real sz: sep ? host.s : host.sizeFor(center)
    width: sep ? (host.vertical ? host.s : (Th.dock3d ? 4 : 1)) : (host.vertical ? host.s * host.zoom : sz)
    height: sep ? (host.vertical ? 1 : host.s) : (host.vertical ? sz : host.s * host.zoom)
    Behavior on sz { enabled: Th.animations; NumberAnimation { duration: 90 } }

    Rectangle {   // separador
        visible: di.sep && !Th.dock3d
        anchors.centerIn: parent
        width: host.vertical ? host.s * 0.7 : 1
        height: host.vertical ? 1 : host.s * 0.7
        color: Th.edge
    }

    Item {
        id: iconBox
        visible: !di.sep
        width: di.sz; height: di.sz
        anchors.bottom: host.vertical ? undefined : parent.bottom
        anchors.verticalCenter: host.vertical ? parent.verticalCenter : undefined
        x: host.vertical ? (Th.dockPos === "left" ? 0 : parent.width - width) : 0
        property real jump: 0
        transform: Translate { y: host.vertical ? 0 : -iconBox.jump; x: host.vertical ? (Th.dockPos === "left" ? iconBox.jump : -iconBox.jump) : 0 }
        Ico {
            id: ico
            anchors.fill: parent
            size: host.s * 1.6
            name: di.item.icon || ""
            visible: di.item.kind !== "launchpad" || status === Image.Ready
        }
        // reflejo en el estante 3D
        Ico {
            visible: Th.dock3d && !host.vertical
            width: parent.width; height: parent.height
            y: parent.height + 1
            size: host.s * 1.6
            name: di.item.icon || ""
            opacity: 0.22
            transform: Scale { origin.y: di.sz / 2; yScale: -0.55 }
        }
        SequentialAnimation {
            id: bounce
            loops: 2
            NumberAnimation { target: iconBox; property: "jump"; to: host.s * 0.45; duration: 220; easing.type: Easing.OutQuad }
            NumberAnimation { target: iconBox; property: "jump"; to: 0; duration: 260; easing.type: Easing.OutBounce }
        }
    }
    // punto de «abierta»
    Rectangle {
        visible: di.running
        width: 5; height: 5; radius: 2.5
        color: Th.dock3d ? "#ffffff" : Th.text
        opacity: 0.9
        x: host.vertical ? (Th.dockPos === "left" ? -7 : parent.width + 2) : (parent.width - width) / 2
        y: host.vertical ? (parent.height - height) / 2 : parent.height + (Th.dock3d ? host.shelf * 0.35 : 3)
    }
    // nombre encima
    Rectangle {
        visible: ma.containsMouse && !di.sep
        width: tip.implicitWidth + 20; height: 26; radius: 13
        color: Th.glassStrong
        border.color: Th.edge
        x: host.vertical ? (Th.dockPos === "left" ? di.sz + 10 : -width - 10) : (parent.width - width) / 2
        y: host.vertical ? (parent.height - height) / 2 : parent.height - di.sz - height - 10
        Txt { id: tip; anchors.centerIn: parent; text: di.item.name || ""; font.pixelSize: 12 }
    }
    MouseArea {
        id: ma
        anchors.fill: iconBox
        enabled: !di.sep
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: function (m) {
            if (m.button === Qt.RightButton) {
                var p = di.mapToItem(null, di.width / 2, 0)
                Ux.toggle("dockmenu", p.x, p.y, di.item)
                return
            }
            if (di.item.kind === "app" && !di.running && Th.animations) bounce.start()
            di.host.open(di.item)
        }
    }
}
