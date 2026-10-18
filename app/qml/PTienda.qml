// Tienda: el catálogo de RROS (pacman o Flathub), con el progreso de la instalación en la misma ventana
import QtQuick
import QtQuick.Layouts
import "."

RPage {
    id: p
    title: "Tienda"
    subtitle: "Aplicaciones probadas para RROS. Se instalan con su logotipo y quedan en el Launchpad."
    icon: "qrc:/icons/app-tienda.svg"
    property var apps: []
    property string cat: ""
    property string q: ""
    property string working: ""       // id de la app que se instala o quita
    property string log: ""
    readonly property var cats: [["", "Todas"], ["internet", "Internet"], ["office", "Oficina"], ["media", "Música y vídeo"],
                                 ["graphics", "Gráficos"], ["dev", "Programación"], ["utility", "Utilidades"], ["games", "Juegos"], ["system", "Sistema"]]

    function refresh() { Ui.api("GET", "/api/apps", {}, function (r) { apps = Array.isArray(r) ? r : [] }) }
    function shown_() {
        return apps.filter(function (a) {
            return (cat === "" || a.category === cat) &&
                   (q === "" || (a.name + " " + a.desc).toLowerCase().indexOf(q.toLowerCase()) >= 0)
        })
    }
    function act(a, action) {
        working = a.id; log = ""
        Sys.stream(["pkexec", "rros-apps", action, a.id],
                   function (line) { p.log = line },
                   function (code) {
                       p.working = ""
                       if (code === 0) Ui.toast(a.name + (action === "install" ? " instalada" : " quitada"), false)
                       else if (code === 126 || code === 127) Ui.toast("Hace falta la contraseña de un administrador", true)
                       else Ui.toast("No se pudo: " + p.log, true)
                       p.refresh()
                   })
    }
    function iconOf(a) { return "image://icon/" + (a.cmd || a.id) }
    onShown: if (apps.length === 0) refresh()

    Note {
        visible: Sys.live
        kind: "warn"
        text: "Estás en el sistema en vivo: lo que instales se guarda en la memoria y se pierde al apagar. Instala RROS para conservarlo."
    }
    RowLayout {
        Layout.fillWidth: true
        Field { Layout.fillWidth: true; placeholderText: "Buscar aplicaciones"; onTextChanged: p.q = text }
    }
    Flow {
        Layout.fillWidth: true
        spacing: 6
        Repeater {
            model: p.cats.filter(function (c) { return c[0] === "" || p.apps.some(function (a) { return a.category === c[0] }) })
            Rectangle {
                property bool on: p.cat === modelData[0]
                implicitWidth: ct.implicitWidth + 24; implicitHeight: 28; radius: 14
                color: on ? Ui.accent : Ui.field
                border.color: on ? "transparent" : Ui.line
                Txt { id: ct; anchors.centerIn: parent; text: modelData[1]; color: on ? Ui.accentText : Ui.textColor; font.pixelSize: Ui.fs - 1 }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: p.cat = modelData[0] }
            }
        }
    }
    GridLayout {
        Layout.fillWidth: true
        columns: width > 640 ? 2 : 1
        columnSpacing: 12
        rowSpacing: 12
        Repeater {
            model: p.shown_()
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 92
                radius: 14
                color: Ui.card
                border.color: Ui.line
                RowLayout {
                    anchors { fill: parent; margins: 14 }
                    spacing: 14
                    Image { source: p.iconOf(modelData); sourceSize: Qt.size(56, 56); Layout.preferredWidth: 56; Layout.preferredHeight: 56; asynchronous: true }
                    ColumnLayout {
                        spacing: 2
                        Layout.fillWidth: true
                        Txt { text: modelData.name; font.weight: Font.DemiBold; Layout.fillWidth: true }
                        Txt { text: p.working === modelData.id ? p.log : modelData.desc; color: Ui.muted; font.pixelSize: Ui.fs - 1; wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight; Layout.fillWidth: true }
                        Txt { text: modelData.installed ? "Instalada" : (modelData.pkg ? "Paquete de Arch" : "Flathub"); color: modelData.installed ? Ui.ok : Ui.muted; font.pixelSize: 11 }
                    }
                    ColumnLayout {
                        spacing: 6
                        Btn {
                            text: modelData.installed ? "Abrir" : "Instalar"
                            kind: modelData.installed ? "normal" : "primary"
                            busy: p.working === modelData.id
                            enabled2: p.working === ""
                            onClicked: modelData.installed ? Sys.detached([modelData.cmd || modelData.id]) : p.act(modelData, "install")
                        }
                        Btn { visible: modelData.installed && !modelData.shipped; text: "Quitar"; kind: "danger"; enabled2: p.working === ""; onClicked: p.act(modelData, "remove") }
                    }
                }
            }
        }
    }
}
