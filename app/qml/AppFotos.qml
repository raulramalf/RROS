// Lily Fotos: visor de imágenes. Sin argumento muestra una galería de ~/Imágenes (y sus subcarpetas directas); con una
// imagen la abre a pantalla completa con anterior/siguiente, zoom, girar, poner de fondo y papelera.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

ApplicationWindow {
    id: win
    width: 1040; height: 680
    visible: true
    title: viewing ? photos[idx].name : "Fotos"
    color: viewing ? "#0d0507" : Ui.window
    font.family: Ui.font
    readonly property var filters: ["*.png", "*.jpg", "*.jpeg", "*.webp", "*.gif", "*.bmp", "*.svg"]
    readonly property string root: Sys.env("HOME") + "/Imágenes"
    property var photos: []
    property int idx: -1
    readonly property bool viewing: idx >= 0 && idx < photos.length
    property real zoom: 1
    property int angle: 0

    function load(dir) {
        var l = Sys.listDir(dir, filters)
        Sys.entries(dir, false).forEach(function (e) { if (e.dir) l = l.concat(Sys.listDir(e.path, filters)) })
        l.sort(function (a, b) { return b.modified - a.modified })
        photos = l
    }
    function show(i) { idx = (i + photos.length) % photos.length; zoom = 1; angle = 0 }
    function trash() {
        var p = photos[idx].path
        Sys.run(["gio", "trash", p], function () {
            var l = photos.slice(); l.splice(idx, 1); photos = l
            if (!photos.length) idx = -1; else show(Math.min(idx, photos.length - 1))
        })
    }

    Component.onCompleted: {
        Ui.reload()
        if (appArg !== "" && Sys.exists(appArg)) {
            var dir = appArg.substring(0, appArg.lastIndexOf("/"))
            photos = Sys.listDir(dir, filters).sort(function (a, b) { return a.name.localeCompare(b.name) })
            for (var i = 0; i < photos.length; i++) if (photos[i].path === appArg) show(i)
        } else load(root)
        if (shotPath !== "") shotT.start()
    }
    Timer { id: shotT; interval: 1800; onTriggered: { Sys.saveShot(win, shotPath); Qt.quit() } }
    Shortcut { sequence: "Right"; enabled: win.viewing; onActivated: win.show(win.idx + 1) }
    Shortcut { sequence: "Left"; enabled: win.viewing; onActivated: win.show(win.idx - 1) }
    Shortcut { sequence: "Escape"; onActivated: if (win.viewing) win.idx = -1; else Qt.quit() }
    Shortcut { sequence: StandardKey.ZoomIn; enabled: win.viewing; onActivated: win.zoom = Math.min(8, win.zoom * 1.25) }
    Shortcut { sequence: StandardKey.ZoomOut; enabled: win.viewing; onActivated: win.zoom = Math.max(0.2, win.zoom / 1.25) }
    Shortcut { sequence: "R"; enabled: win.viewing; onActivated: win.angle += 90 }
    Shortcut { sequences: [StandardKey.Delete]; enabled: win.viewing; onActivated: win.trash() }
    Shortcut { sequence: "F11"; onActivated: win.visibility = win.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen }

    // ---- galería ----
    ColumnLayout {
        anchors { fill: parent; margins: 18 }
        visible: !win.viewing
        spacing: 12
        RowLayout {
            Txt { text: "Fotos"; font.pixelSize: 24; font.weight: Font.Bold }
            Txt { text: win.photos.length + " imágenes en Imágenes"; color: Ui.muted; Layout.fillWidth: true; Layout.leftMargin: 8 }
            Btn { text: "Abrir carpeta"; onClicked: Sys.detached(["rros-ajustes", "--app", "archivos", win.root]) }
        }
        GridView {
            id: gal
            Layout.fillWidth: true; Layout.fillHeight: true
            clip: true
            model: win.photos
            cellWidth: width / Math.max(1, Math.floor(width / 190)); cellHeight: cellWidth * 0.75
            ScrollBar.vertical: ScrollBar {}
            delegate: Item {
                width: gal.cellWidth; height: gal.cellHeight
                Rectangle {
                    anchors { fill: parent; margins: 4 }
                    radius: 10; clip: true; color: Ui.card
                    border.color: gm.containsMouse ? Ui.accent : Ui.line; border.width: gm.containsMouse ? 2 : 1
                    Image { anchors { fill: parent; margins: 2 } asynchronous: true; fillMode: Image.PreserveAspectCrop
                            source: "file://" + modelData.path; sourceSize: Qt.size(360, 270) }
                    MouseArea { id: gm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.show(index) }
                }
            }
            Txt { anchors.centerIn: parent; visible: gal.count === 0; color: Ui.muted; text: "Aún no hay imágenes en Imágenes. Las capturas se guardan en Imágenes/Capturas." }
        }
    }

    // ---- visor ----
    Item {
        anchors.fill: parent
        visible: win.viewing
        Flickable {
            id: fl
            anchors.fill: parent
            contentWidth: Math.max(width, pic.paintedWidth * win.zoom); contentHeight: Math.max(height, pic.paintedHeight * win.zoom)
            clip: true
            Image {
                id: pic
                width: fl.width; height: fl.height
                anchors.centerIn: parent
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                source: win.viewing ? "file://" + win.photos[win.idx].path : ""
                scale: win.zoom; rotation: win.angle
                Behavior on rotation { NumberAnimation { duration: 160 } }
            }
            WheelHandler { onWheel: function (ev) { win.zoom = Math.max(0.2, Math.min(8, win.zoom * (ev.angleDelta.y > 0 ? 1.12 : 1 / 1.12))) } }
            TapHandler { onDoubleTapped: win.zoom = win.zoom === 1 ? 2 : 1 }
        }
        // barra inferior flotante
        Rectangle {
            anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 18 }
            width: bar.implicitWidth + 20; height: 48; radius: 24
            color: "#cc1a0a10"; border.color: "#33ffffff"
            RowLayout {
                id: bar
                anchors.centerIn: parent
                spacing: 6
                Btn { text: "Galería"; onClicked: win.idx = -1 }
                Btn { text: "‹"; onClicked: win.show(win.idx - 1) }
                Txt { text: (win.idx + 1) + " / " + win.photos.length; color: "#f6ecdf"; Layout.leftMargin: 6; Layout.rightMargin: 6 }
                Btn { text: "›"; onClicked: win.show(win.idx + 1) }
                Btn { text: "−"; onClicked: win.zoom = Math.max(0.2, win.zoom / 1.25) }
                Btn { text: Math.round(win.zoom * 100) + " %"; onClicked: win.zoom = 1 }
                Btn { text: "+"; onClicked: win.zoom = Math.min(8, win.zoom * 1.25) }
                Btn { text: "Girar"; onClicked: win.angle += 90 }
                Btn { text: "Poner de fondo"; onClicked: Ui.setKey("ui.wallpaper", win.photos[win.idx].path) }
                Btn { text: "Papelera"; kind: "danger"; onClicked: win.trash() }
            }
        }
    }
}
