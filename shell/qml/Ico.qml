// Icono de una aplicación por su nombre en el tema de iconos (o su ruta)
import QtQuick

Image {
    property string name
    property int size: 48
    source: name ? "image://icon/" + name : ""
    sourceSize: Qt.size(size * 2, size * 2)
    width: size
    height: size
    smooth: true
    mipmap: true
    asynchronous: true
}
