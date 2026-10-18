// Inicio de sesión de RROS (SDDM): reloj arriba, la persona en el centro y los botones de apagar abajo.
import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: root
    width: 1280
    height: 800
    color: "#140609"
    property color accent: config.accent ? config.accent : "#c8243c"
    property int userIndex: userModel.lastIndex >= 0 ? userModel.lastIndex : 0
    property int sessionIndex: sessionModel.lastIndex >= 0 ? sessionModel.lastIndex : 0
    property string userName: ""
    property string realName: ""
    property string error: ""

    Image {
        anchors.fill: parent
        source: config.background ? config.background : ""
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
    }
    Rectangle { anchors.fill: parent; color: "#000000"; opacity: 0.35 }

    // los datos de la persona elegida (el modelo no da acceso directo por índice)
    Repeater {
        model: userModel
        Item {
            visible: false
            Component.onCompleted: if (index === root.userIndex) { root.userName = model.name; root.realName = model.realName || model.name }
        }
    }

    // ---- reloj ----
    Column {
        anchors { top: parent.top; topMargin: parent.height * 0.08; horizontalCenter: parent.horizontalCenter }
        spacing: 2
        Text {
            id: time
            anchors.horizontalCenter: parent.horizontalCenter
            color: "#ffffff"
            font.family: "Inter"
            font.pixelSize: 84
            font.weight: Font.DemiBold
            text: Qt.formatTime(new Date(), "HH:mm")
        }
        Text {
            id: date
            anchors.horizontalCenter: parent.horizontalCenter
            color: "#f6ecdf"
            font.family: "Inter"
            font.pixelSize: 20
            text: new Date().toLocaleDateString(Qt.locale("es_ES"), "dddd, d 'de' MMMM")
        }
        Timer {
            interval: 1000; running: true; repeat: true
            onTriggered: { time.text = Qt.formatTime(new Date(), "HH:mm"); date.text = new Date().toLocaleDateString(Qt.locale("es_ES"), "dddd, d 'de' MMMM") }
        }
    }

    // ---- persona y contraseña ----
    Column {
        anchors { centerIn: parent; verticalCenterOffset: parent.height * 0.08 }
        spacing: 16

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 112; height: 112; radius: 56
            color: Qt.rgba(1, 1, 1, 0.16)
            border.color: Qt.rgba(1, 1, 1, 0.35)
            border.width: 2
            Text {
                anchors.centerIn: parent
                text: root.realName.length ? root.realName.charAt(0).toUpperCase() : "R"
                color: "#ffffff"
                font.family: "Inter"
                font.pixelSize: 48
                font.weight: Font.DemiBold
            }
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.realName
            color: "#ffffff"
            font.family: "Inter"
            font.pixelSize: 22
            font.weight: Font.DemiBold
        }
        // cambiar de persona (si hay varias)
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 8
            visible: userModel.count > 1 && userModel.count <= 6
            Repeater {
                model: userModel
                Rectangle {
                    width: label.implicitWidth + 20; height: 26; radius: 13
                    color: index === root.userIndex ? root.accent : Qt.rgba(1, 1, 1, 0.14)
                    Text { id: label; anchors.centerIn: parent; text: model.realName || model.name; color: "#ffffff"; font.family: "Inter"; font.pixelSize: 13 }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: { root.userIndex = index; root.userName = model.name; root.realName = model.realName || model.name; password.forceActiveFocus() }
                    }
                }
            }
        }
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 8
            TextField {
                id: password
                width: 260
                height: 40
                echoMode: TextInput.Password
                placeholderText: "Contraseña"
                color: "#ffffff"
                placeholderTextColor: "#e6d6c6"
                font.family: "Inter"
                font.pixelSize: 15
                leftPadding: 16
                focus: true
                background: Rectangle { radius: 20; color: Qt.rgba(1, 1, 1, 0.16); border.color: password.activeFocus ? root.accent : Qt.rgba(1, 1, 1, 0.3); border.width: password.activeFocus ? 2 : 1 }
                Keys.onReturnPressed: root.login()
                Keys.onEnterPressed: root.login()
            }
            Rectangle {
                width: 40; height: 40; radius: 20
                color: root.accent
                Text { anchors.centerIn: parent; text: "→"; color: "#ffffff"; font.pixelSize: 20 }
                MouseArea { anchors.fill: parent; onClicked: root.login() }
            }
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.error !== "" ? root.error : (keyboard.capsLock ? "Bloq Mayús está activado" : " ")
            color: root.error !== "" ? "#ff8fa3" : "#fbbf24"
            font.family: "Inter"
            font.pixelSize: 13
        }
    }

    function login() {
        root.error = ""
        sddm.login(root.userName, password.text, root.sessionIndex)
    }
    Connections {
        target: sddm
        function onLoginFailed() { root.error = "Contraseña incorrecta"; password.text = ""; password.forceActiveFocus() }
    }

    // ---- abajo: sesión y energía ----
    Row {
        anchors { bottom: parent.bottom; right: parent.right; margins: 24 }
        spacing: 10
        Repeater {
            model: [["Reposo", "suspend"], ["Reiniciar", "reboot"], ["Apagar", "poweroff"]]
            Rectangle {
                visible: modelData[1] === "suspend" ? sddm.canSuspend : modelData[1] === "reboot" ? sddm.canReboot : sddm.canPowerOff
                width: t.implicitWidth + 28; height: 34; radius: 17
                color: ma.containsMouse ? Qt.rgba(1, 1, 1, 0.25) : Qt.rgba(1, 1, 1, 0.14)
                Text { id: t; anchors.centerIn: parent; text: modelData[0]; color: "#ffffff"; font.family: "Inter"; font.pixelSize: 13 }
                MouseArea {
                    id: ma; anchors.fill: parent; hoverEnabled: true
                    onClicked: modelData[1] === "suspend" ? sddm.suspend() : modelData[1] === "reboot" ? sddm.reboot() : sddm.powerOff()
                }
            }
        }
    }
    Row {
        anchors { bottom: parent.bottom; left: parent.left; margins: 24 }
        spacing: 10
        Image { source: "rros.png"; width: 30; height: 30; smooth: true; mipmap: true }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: "RROS"
            color: "#ffffff"; font.family: "Inter"; font.pixelSize: 15; font.weight: Font.Bold
        }
    }
    Component.onCompleted: password.forceActiveFocus()
}
