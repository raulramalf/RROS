// Fondo en vídeo (mp4, webm, mkv…) o GIF animado: en bucle, sin sonido, recortado a la pantalla. Se pausa con la
// batería si así se elige en Ajustes (ui.wallpaper_pause_battery). Va en un Loader: sin QtMultimedia, no se carga y
// queda el fondo fijo.
import QtQuick
import QtMultimedia
import "."

Item {
    id: vw
    property string path: ""
    readonly property bool gif: /\.gif$/i.test(path)
    readonly property bool paused: Th.pauseOnBattery && Sy.battery >= 0 && !Sy.charging
    AnimatedImage {
        anchors.fill: parent
        visible: vw.gif
        source: vw.gif ? "file://" + vw.path : ""
        fillMode: Image.PreserveAspectCrop
        playing: !vw.paused
        cache: false
    }
    MediaPlayer {
        id: mp
        source: vw.gif || vw.path === "" ? "" : "file://" + vw.path
        loops: MediaPlayer.Infinite
        videoOutput: out
        Component.onCompleted: if (!vw.gif && vw.path !== "") play()
        onSourceChanged: if (source != "") play()
    }
    onPausedChanged: if (!gif) { if (paused) mp.pause(); else mp.play() }
    VideoOutput {
        id: out
        anchors.fill: parent
        visible: !vw.gif
        fillMode: VideoOutput.PreserveAspectCrop
    }
}
