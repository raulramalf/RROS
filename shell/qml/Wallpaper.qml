// Fondo de escritorio (limpio: sin iconos). Clic derecho: menú del escritorio.
import QtQuick
import "."

Rectangle {
    color: Th.pal.bg
    Image {
        anchors.fill: parent
        source: Th.wallpaper ? "file://" + Th.wallpaper : ""
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        cache: false
        sourceSize: Qt.size(parent.width, parent.height)
        visible: video.status !== Loader.Ready
    }
    // fondo en vídeo o GIF (anime, paisajes…) y escenas animadas encima del fondo fijo
    Loader {
        id: video
        anchors.fill: parent
        active: Th.wallpaperVideo !== "" && Sh.exists(Th.wallpaperVideo)
        source: "VideoWall.qml"
        onLoaded: item.path = Qt.binding(function () { return Th.wallpaperVideo })
    }
    Loader {
        anchors.fill: parent
        active: Th.wallpaperLive !== "off" && Th.animations
        source: "LiveWall.qml"
        onLoaded: item.scene = Qt.binding(function () { return Th.wallpaperLive })
    }
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton | Qt.LeftButton
        onClicked: function (m) {
            if (m.button === Qt.RightButton) Ux.toggle("desktopmenu", m.x, m.y)
            else Ux.close()
        }
    }
}
