// Menú Lily de la barra superior, como el menú de la manzana de macOS
import QtQuick
import QtQuick.Layouts
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PlasmaComponents3
import org.kde.plasma.plasma5support as P5Support
import org.kde.kirigami as Kirigami

PlasmoidItem {
    id: root
    property bool live: false

    preferredRepresentation: compactRepresentation
    toolTipMainText: "Lily"
    toolTipSubText: "Este equipo, ajustes y apagar"
    Plasmoid.icon: "rros-symbolic"

    P5Support.DataSource {
        id: exe
        engine: "executable"
        connectedSources: []
        onNewData: function (source, data) {
            if (source === "cat /proc/cmdline")
                root.live = String(data["stdout"]).indexOf("archisobasedir") >= 0
            disconnectSource(source)
        }
    }
    function run(cmd) {
        exe.connectSource(cmd)
        root.expanded = false
    }
    Component.onCompleted: exe.connectSource("cat /proc/cmdline")

    compactRepresentation: MouseArea {
        id: compact
        Layout.minimumWidth: Kirigami.Units.iconSizes.medium
        hoverEnabled: true
        onClicked: root.expanded = !root.expanded
        Kirigami.Icon {
            anchors.centerIn: parent
            width: Math.min(parent.height * 0.62, Kirigami.Units.iconSizes.smallMedium)
            height: width
            source: "rros-symbolic"
            active: compact.containsMouse || root.expanded
        }
    }

    fullRepresentation: ColumnLayout {
        Layout.preferredWidth: Kirigami.Units.gridUnit * 15
        spacing: 0

        component Item_: PlasmaComponents3.ItemDelegate {
            property string cmd
            Layout.fillWidth: true
            onClicked: root.run(cmd)
        }

        Item_ { text: "Acerca de este equipo"; icon.name: "rros"; cmd: "rros-ajustes --pagina general" }
        PlasmaComponents3.MenuSeparator { Layout.fillWidth: true }
        Item_ { text: "Ajustes…"; icon.name: "rros-settings"; cmd: "rros-ajustes" }
        Item_ { text: "Tienda…"; icon.name: "rros-apps"; cmd: "rros-ajustes --pagina tienda" }
        Item_ { text: "Actualizaciones…"; icon.name: "rros-updates"; cmd: "rros-ajustes --pagina actualizaciones" }
        Item_ { visible: root.live; text: "Instalar Lily…"; icon.name: "rros-install"; cmd: "rros-ajustes --pagina instalar" }
        PlasmaComponents3.MenuSeparator { Layout.fillWidth: true }
        Item_ { text: "Forzar salida…"; icon.name: "process-stop"; cmd: "qdbus6 org.kde.KWin /KWin killWindow" }
        Item_ { text: "Monitor del sistema"; icon.name: "utilities-system-monitor"; cmd: "plasma-systemmonitor" }
        PlasmaComponents3.MenuSeparator { Layout.fillWidth: true }
        Item_ { text: "Reposo"; icon.name: "system-suspend"; cmd: "systemctl suspend" }
        Item_ { text: "Reiniciar…"; icon.name: "system-reboot"; cmd: "qdbus6 org.kde.LogoutPrompt /LogoutPrompt promptReboot" }
        Item_ { text: "Apagar…"; icon.name: "system-shutdown"; cmd: "qdbus6 org.kde.LogoutPrompt /LogoutPrompt promptShutDown" }
        PlasmaComponents3.MenuSeparator { Layout.fillWidth: true }
        Item_ { text: "Bloquear la pantalla"; icon.name: "system-lock-screen"; cmd: "loginctl lock-session" }
        Item_ { text: "Cerrar sesión…"; icon.name: "system-log-out"; cmd: "qdbus6 org.kde.LogoutPrompt /LogoutPrompt promptLogout" }
    }
}
