import QtQuick
import QtQuick.Controls
import "."

TextField {
    id: f
    implicitWidth: 220
    implicitHeight: 30
    color: Ui.textColor
    placeholderTextColor: Ui.muted
    selectionColor: Ui.accent
    selectedTextColor: Ui.accentText
    font.family: Ui.font
    font.pixelSize: Ui.fs
    leftPadding: 10
    rightPadding: 10
    background: Rectangle {
        radius: 8
        color: Ui.field
        border.color: f.activeFocus ? Ui.accent : Ui.line
        border.width: f.activeFocus ? 2 : 1
    }
}
