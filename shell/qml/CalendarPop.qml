// Calendario del reloj de la barra
import QtQuick
import QtQuick.Layouts
import "."

Glass {
    id: cal
    strong: true
    width: 300
    height: col.implicitHeight + 28
    radius: 18
    property date today: new Date()
    property int month: today.getMonth()
    property int year: today.getFullYear()
    readonly property var names: ["enero", "febrero", "marzo", "abril", "mayo", "junio", "julio", "agosto", "septiembre", "octubre", "noviembre", "diciembre"]
    function cells() {
        var first = new Date(year, month, 1), start = (first.getDay() + 6) % 7, days = new Date(year, month + 1, 0).getDate(), out = []
        for (var i = 0; i < start; i++) out.push(0)
        for (var d = 1; d <= days; d++) out.push(d)
        return out
    }
    ColumnLayout {
        id: col
        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 14 }
        spacing: 8
        Txt { text: cal.today.toLocaleDateString(Qt.locale("es_ES"), "dddd") ; color: Th.accent; font.weight: Font.DemiBold; font.capitalization: Font.Capitalize }
        Txt { text: Qt.formatDate(cal.today, "d") + " de " + cal.names[cal.today.getMonth()]; font.pixelSize: 22; font.weight: Font.Bold }
        RowLayout {
            Layout.fillWidth: true
            Txt { text: cal.names[cal.month] + " " + cal.year; font.weight: Font.DemiBold; font.capitalization: Font.Capitalize; Layout.fillWidth: true }
            Txt { text: "‹"; font.pixelSize: 18; MouseArea { anchors.fill: parent; anchors.margins: -6; onClicked: { if (cal.month === 0) { cal.month = 11; cal.year-- } else cal.month-- } } }
            Txt { text: "›"; font.pixelSize: 18; Layout.leftMargin: 14; MouseArea { anchors.fill: parent; anchors.margins: -6; onClicked: { if (cal.month === 11) { cal.month = 0; cal.year++ } else cal.month++ } } }
        }
        Grid {
            columns: 7
            spacing: 2
            Repeater {
                model: ["L", "M", "X", "J", "V", "S", "D"]
                Txt { width: 36; height: 22; text: modelData; horizontalAlignment: Text.AlignHCenter; color: Th.muted; font.pixelSize: 11 }
            }
            Repeater {
                model: cal.cells()
                Rectangle {
                    property bool isToday: modelData === cal.today.getDate() && cal.month === cal.today.getMonth() && cal.year === cal.today.getFullYear()
                    width: 36; height: 30; radius: 15
                    color: isToday ? Th.accent : "transparent"
                    Txt { anchors.centerIn: parent; text: modelData > 0 ? modelData : ""; color: parent.isToday ? "#ffffff" : Th.text; font.pixelSize: 12 }
                }
            }
        }
    }
}
