// Fondo de escritorio con los archivos de la carpeta Escritorio encima. Clic derecho: menú del escritorio.
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
        active: Th.wallpaperLive !== "off" && Th.liveScenes
        source: "LiveWall.qml"
        onLoaded: item.scene = Qt.binding(function () { return Th.wallpaperLive })
    }
    // los archivos de ~/Escritorio (con su menú, selección y arrastrar); clic derecho en un hueco: menú del escritorio
    DesktopIcons {
        anchors.fill: parent
        show: Th.desktopIcons
        onBackgroundMenu: function (x, y) { Ux.toggle("desktopmenu", x, y) }
    }
}
