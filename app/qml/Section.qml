// Etiqueta encima de una tarjeta
import QtQuick
import QtQuick.Layouts
import "."

Txt {
    Layout.fillWidth: true
    Layout.topMargin: 4
    Layout.bottomMargin: -10
    leftPadding: 4
    color: Ui.muted
    font.pixelSize: Ui.fs - 1
    font.weight: Font.DemiBold
    font.capitalization: Font.AllUppercase
    font.letterSpacing: 0.6
}
