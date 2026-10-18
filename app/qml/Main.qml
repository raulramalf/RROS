// Ventana principal de RROS Ajustes: barra lateral con las secciones y la página a la derecha.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "."

ApplicationWindow {
    id: win
    width: welcomeMode ? 860 : 1080
    height: welcomeMode ? 600 : 720
    minimumWidth: welcomeMode ? 760 : 860
    minimumHeight: 560
    visible: true
    title: welcomeMode ? "Bienvenido a RROS" : "RROS Ajustes"
    color: Ui.window
    font.family: Ui.font

    // [id, etiqueta, icono, archivo, grupo]
    readonly property var sections: [
        ["general", "General", "sec-general", "PGeneral.qml", 0],
        ["apariencia", "Apariencia", "sec-apariencia", "PApariencia.qml", 0],
        ["escritorio", "Escritorio y Dock", "sec-escritorio", "PEscritorio.qml", 0],
        ["red", "Red e Internet", "sec-red", "PRed.qml", 1],
        ["bluetooth", "Bluetooth", "sec-bluetooth", "PBluetooth.qml", 1],
        ["sonido", "Sonido", "sec-sonido", "PSonido.qml", 1],
        ["pantalla", "Pantallas", "sec-pantalla", "PPantalla.qml", 1],
        ["raton", "Ratón y panel táctil", "sec-raton", "PRaton.qml", 1],
        ["teclado", "Teclado y atajos", "sec-teclado", "PTeclado.qml", 1],
        ["energia", "Batería y energía", "sec-energia", "PEnergia.qml", 1],
        ["dispositivos", "Dispositivos", "app-dispositivos", "PDispositivos.qml", 1],
        ["fecha", "Fecha, hora e idioma", "sec-fecha", "PFecha.qml", 2],
        ["accesibilidad", "Accesibilidad", "sec-accesibilidad", "PAccesibilidad.qml", 2],
        ["seguridad", "Seguridad y privacidad", "app-seguridad", "PSeguridad.qml", 2],
        ["tienda", "Tienda", "app-tienda", "PTienda.qml", 3],
        ["actualizaciones", "Actualizaciones", "app-actualizar", "PActualizaciones.qml", 3],
        ["avanzado", "Avanzado", "sec-avanzado", "PAvanzado.qml", 3],
        ["instalar", "Instalar RROS", "app-instalar", "PInstalar.qml", 4]
    ]
    property string current: "general"
    property string filter: ""

    function go(id) {
        var aliases = { "temas": "apariencia", "dock": "escritorio", "wifi": "red", "audio": "sonido", "archivos": "general",
                        "copias": "avanzado", "energía": "energia", "seguridad": "seguridad" }
        if (aliases[id]) id = aliases[id]
        if (id.indexOf("cat:") === 0) id = "avanzado"
        for (var i = 0; i < sections.length; i++)
            if (sections[i][0] === id) { current = id; return }
    }

    Component.onCompleted: {
        Ui.reload(function () {
            if (startPage !== "") go(startPage)
            if (shotPath !== "") shotTimer.start()
        })
    }
    Timer {
        // --captura: espera a que se dibuje y guarda la ventana (pruebas y documentación)
        id: shotTimer
        interval: 1800
        onTriggered: { Sys.saveShot(win, shotPath); Qt.quit() }
    }
    Connections {
        target: Ui
        function onToast(text, error) { toast.show(text, error) }
    }

    Loader {
        anchors.fill: parent
        active: welcomeMode
        source: "Welcome.qml"
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0
        visible: !welcomeMode

        // ---- barra lateral ----
        Rectangle {
            Layout.preferredWidth: 248
            Layout.fillHeight: true
            color: Ui.sidebar
            Rectangle { anchors { right: parent.right; top: parent.top; bottom: parent.bottom } width: 1; color: Ui.line }
            ColumnLayout {
                anchors { fill: parent; margins: 12; topMargin: 16 }
                spacing: 8
                RowLayout {
                    spacing: 10
                    Layout.leftMargin: 6
                    Image { source: "qrc:/icons/app-rros.svg"; sourceSize: Qt.size(30, 30) }
                    ColumnLayout {
                        spacing: 0
                        Txt { text: "RROS Ajustes"; font.weight: Font.Bold; font.pixelSize: 15 }
                        Txt { text: Ui.st ? "RROS " + Ui.st.version + " · " + Ui.st.profile : ""; color: Ui.muted; font.pixelSize: 11 }
                    }
                }
                Field {
                    Layout.fillWidth: true
                    Layout.topMargin: 6
                    placeholderText: "Buscar"
                    onTextChanged: win.filter = text.toLowerCase()
                }
                ListView {
                    id: nav
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 1
                    boundsBehavior: Flickable.StopAtBounds
                    model: win.sections.filter(function (s) {
                        if (s[0] === "instalar" && !Sys.live) return false
                        return win.filter === "" || s[1].toLowerCase().indexOf(win.filter) >= 0
                    })
                    section.property: "modelData.4"
                    delegate: Rectangle {
                        width: nav.width
                        height: 34 + (index > 0 && nav.model[index - 1][4] !== modelData[4] ? 10 : 0)
                        color: "transparent"
                        property bool sel: win.current === modelData[0]
                        Rectangle {
                            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
                            height: 34
                            radius: 8
                            color: sel ? Ui.accent : (hov.containsMouse ? Ui.cardHover : "transparent")
                            opacity: sel ? 0.9 : 1
                        }
                        RowLayout {
                            anchors { left: parent.left; right: parent.right; bottom: parent.bottom; leftMargin: 8 }
                            height: 34
                            spacing: 10
                            Image { source: "qrc:/icons/" + modelData[2] + ".svg"; sourceSize: Qt.size(24, 24); Layout.preferredWidth: 24; Layout.preferredHeight: 24 }
                            Txt { text: modelData[1]; color: sel ? Ui.accentText : Ui.textColor; Layout.fillWidth: true }
                        }
                        MouseArea { id: hov; anchors.fill: parent; hoverEnabled: true; onClicked: win.current = modelData[0] }
                    }
                }
            }
        }

        // ---- página ----
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Repeater {
                model: win.sections
                Loader {
                    anchors.fill: parent
                    // cada página se crea la primera vez que se abre y luego se conserva
                    property bool wanted: win.current === modelData[0]
                    active: wanted || item !== null
                    visible: wanted
                    source: active ? modelData[3] : ""
                    onWantedChanged: if (wanted && item && item.shown) item.shown()
                    onLoaded: if (item.shown) item.shown()
                }
            }
            Txt {
                anchors.centerIn: parent
                visible: !Ui.loaded
                text: "Cargando…"
                color: Ui.muted
            }
        }
    }

    // ---- avisos ----
    Rectangle {
        id: toast
        property bool error: false
        function show(t, err) { msg.text = t; error = err; opacity = 1; hide.restart() }
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 26 }
        width: Math.min(msg.implicitWidth + 36, win.width - 80)
        height: msg.implicitHeight + 20
        radius: 12
        color: error ? Ui.danger : Ui.card
        border.color: Ui.line
        opacity: 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 180 } }
        Txt { id: msg; anchors.centerIn: parent; width: Math.min(implicitWidth, win.width - 116); wrapMode: Text.WordWrap; color: toast.error ? "#ffffff" : Ui.textColor }
        Timer { id: hide; interval: 4200; onTriggered: toast.opacity = 0 }
    }
}
