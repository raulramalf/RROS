// Bienvenida: el primer inicio de sesión. Aspecto, dock, estilo y, en el sistema en vivo, instalar o probar.
import QtQuick
import QtQuick.Layouts
import "."

Rectangle {
    id: w
    color: Ui.window
    property int step: 0
    readonly property int last: 4

    function finish() {
        Sys.detached(["sh", "-c", "mkdir -p \"$HOME/.config/rros\" && touch \"$HOME/.config/rros/.bienvenida-vista\""])
        Qt.quit()
    }

    // fondo con la aurora del tema
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: Qt.tint(Ui.window, Qt.rgba(Qt.color(Ui.accent).r, Qt.color(Ui.accent).g, Qt.color(Ui.accent).b, 0.18)) }
            GradientStop { position: 1; color: Ui.window }
        }
    }

    ColumnLayout {
        anchors { fill: parent; margins: 40; bottomMargin: 28 }
        spacing: 18

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // 0. hola
            ColumnLayout {
                anchors.centerIn: parent
                visible: w.step === 0
                spacing: 14
                Image {
                    source: "qrc:/icons/app-rros.png"; sourceSize: Qt.size(132, 132); Layout.alignment: Qt.AlignHCenter
                    NumberAnimation on scale { from: 0.85; to: 1; duration: 600; easing.type: Easing.OutBack; running: w.step === 0 }
                }
                Txt { text: "Bienvenido a RROS"; font.pixelSize: 34; font.weight: Font.Black; Layout.alignment: Qt.AlignHCenter }
                Txt { text: "Ligero, tuyo y en español. Vamos a dejarlo a tu gusto en un minuto."; color: Ui.muted; font.pixelSize: 15; Layout.alignment: Qt.AlignHCenter }
                Txt { text: Ui.st ? "Edición " + Ui.st.profile.charAt(0).toUpperCase() + Ui.st.profile.slice(1) + " · RROS " + Ui.st.version : ""; color: Ui.muted; Layout.alignment: Qt.AlignHCenter }
            }

            // 1. aspecto
            ColumnLayout {
                anchors.centerIn: parent
                visible: w.step === 1
                spacing: 18
                Txt { text: "Elige el aspecto"; font.pixelSize: 26; font.weight: Font.Bold; Layout.alignment: Qt.AlignHCenter }
                RowLayout {
                    spacing: 22
                    Layout.alignment: Qt.AlignHCenter
                    Repeater {
                        model: [["light", "Claro", "#f6efe7", "#ffffff"], ["dark", "Oscuro", "#140609", "#2a1018"]]
                        ColumnLayout {
                            spacing: 8
                            Rectangle {
                                width: 200; height: 128; radius: 14; color: modelData[2]
                                border.width: Ui.v("ui.color_scheme") === modelData[0] ? 4 : 1
                                border.color: Ui.v("ui.color_scheme") === modelData[0] ? Ui.accent : Ui.line
                                Rectangle { x: 18; y: 18; width: 130; height: 72; radius: 8; color: modelData[3] }
                                Rectangle { x: 50; y: 104; width: 100; height: 12; radius: 6; color: Ui.accent; opacity: 0.8 }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: Ui.setKey("ui.color_scheme", modelData[0]) }
                            }
                            Txt { text: modelData[1]; Layout.alignment: Qt.AlignHCenter; font.weight: Font.DemiBold }
                        }
                    }
                }
                RowLayout {
                    spacing: 10
                    Layout.alignment: Qt.AlignHCenter
                    Repeater {
                        model: ["#c8243c", "#d9b98f", "#7c5cff", "#0a84ff", "#22c3e6", "#30d158", "#ffb020", "#ff6b3d", "#ff375f", "#bf5af2"]
                        Rectangle {
                            width: 30; height: 30; radius: 15; color: modelData
                            border.width: Ui.v("ui.accent").toLowerCase() === modelData ? 3 : 0
                            border.color: Ui.textColor
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: Ui.setKey("ui.accent", modelData) }
                        }
                    }
                }
            }

            // 2. dock
            ColumnLayout {
                anchors.centerIn: parent
                visible: w.step === 2
                spacing: 18
                Txt { text: "Tu dock: ¿plano o en 3D?"; font.pixelSize: 26; font.weight: Font.Bold; Layout.alignment: Qt.AlignHCenter }
                Txt { text: "El 3D tiene relieve, brillo y el reflejo de los iconos. Se cambia cuando quieras en Ajustes."; color: Ui.muted; Layout.alignment: Qt.AlignHCenter }
                RowLayout {
                    spacing: 22
                    Layout.alignment: Qt.AlignHCenter
                    Repeater {
                        model: [["2d", "2D"], ["3d", "3D"]]
                        ColumnLayout {
                            spacing: 8
                            Rectangle {
                                width: 250; height: 112; radius: 14; color: "transparent"
                                border.width: Ui.v("ui.dock_style") === modelData[0] ? 4 : 1
                                border.color: Ui.v("ui.dock_style") === modelData[0] ? Ui.accent : Ui.line
                                DockPreview { anchors.centerIn: parent; s: 1.2; threeD: modelData[0] === "3d" }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: Ui.setKey("ui.dock_style", modelData[0]) }
                            }
                            Txt { text: modelData[1]; Layout.alignment: Qt.AlignHCenter; font.weight: Font.DemiBold }
                        }
                    }
                }
            }

            // 3. estilo
            ColumnLayout {
                anchors.centerIn: parent
                visible: w.step === 3
                spacing: 18
                Txt { text: "¿Cómo quieres el escritorio?"; font.pixelSize: 26; font.weight: Font.Bold; Layout.alignment: Qt.AlignHCenter }
                RowLayout {
                    spacing: 16
                    Layout.alignment: Qt.AlignHCenter
                    Repeater {
                        model: Ui.st ? Ui.st.desktops : []
                        ColumnLayout {
                            spacing: 8
                            Rectangle {
                                width: 206; height: 128; radius: 14; color: "transparent"
                                border.width: Ui.st.desktop_current === modelData.id ? 4 : 1
                                border.color: Ui.st.desktop_current === modelData.id ? Ui.accent : Ui.line
                                StylePreview { anchors.centerIn: parent; kind: modelData.id }
                                MouseArea {
                                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                    onClicked: Ui.api("POST", "/api/desktop", { name: modelData.id }, function () { Ui.reload() })
                                }
                            }
                            Txt { text: modelData.label; Layout.alignment: Qt.AlignHCenter; font.weight: Font.DemiBold }
                        }
                    }
                }
            }

            // 4. listo
            ColumnLayout {
                anchors.centerIn: parent
                visible: w.step === 4
                spacing: 14
                Image { source: Sys.live ? "qrc:/icons/app-instalar.svg" : "qrc:/icons/app-bienvenida.svg"; sourceSize: Qt.size(110, 110); Layout.alignment: Qt.AlignHCenter }
                Txt { text: Sys.live ? "¿Lo instalamos?" : "¡Todo listo!"; font.pixelSize: 28; font.weight: Font.Bold; Layout.alignment: Qt.AlignHCenter }
                Txt {
                    Layout.preferredWidth: 520; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; elide: Text.ElideNone; color: Ui.muted
                    text: Sys.live ? "Estás probando RROS sin tocar tu disco. Instálalo para que vaya más rápido y se guarden tus cosas; puede ir junto a Windows."
                                   : "La Tienda tiene más aplicaciones, y en Ajustes puedes cambiar todo esto cuando quieras. El Launchpad, en el dock, muestra todas tus aplicaciones."
                }
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 10
                    Btn { visible: Sys.live; text: "Instalar RROS"; kind: "primary"; onClicked: { Sys.detached(["rros-ajustes", "--pagina", "instalar"]); w.finish() } }
                    Btn { visible: !Sys.live; text: "Abrir la Tienda"; onClicked: Sys.detached(["rros-ajustes", "--pagina", "tienda"]) }
                    Btn { visible: !Sys.live; text: "Abrir Ajustes"; onClicked: Sys.detached(["rros-ajustes"]) }
                }
            }
        }

        // puntos y botones
        RowLayout {
            Layout.fillWidth: true
            Row {
                spacing: 8
                Repeater { model: w.last + 1; Rectangle { width: index === w.step ? 22 : 8; height: 8; radius: 4; color: index === w.step ? Ui.accent : Ui.track; Behavior on width { NumberAnimation { duration: 150 } } } }
            }
            Item { Layout.fillWidth: true }
            Btn { text: "Atrás"; visible: w.step > 0; onClicked: w.step-- }
            Btn { text: w.step === w.last ? (Sys.live ? "Seguir probando" : "Empezar") : "Siguiente"; kind: "primary"; onClicked: w.step === w.last ? w.finish() : w.step++ }
        }
    }
}
