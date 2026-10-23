// Escenas animadas sobre el fondo: pétalos de lirio o de sakura, hojas de otoño, nieve, luciérnagas, burbujas de luz,
// cielo estrellado con estrellas fugaces, aurora boreal o lluvia. Con partículas de Qt Quick y animaciones sencillas:
// pocas y ligeras, para que no cuesten CPU. Se paran con la batería si así se elige (y sin aceleración gráfica no
// se cargan: Th.animations).
import QtQuick
import QtQuick.Particles
import "."

Item {
    id: lw
    property string scene: "petalos"
    readonly property bool paused: Th.pauseOnBattery && Sy.battery >= 0 && !Sy.charging
    ParticleSystem { id: ps; running: !lw.paused }

    // ---- pétalos de lirio araña: caen girando y se mecen con el viento ----
    Emitter {
        system: ps; enabled: lw.scene === "petalos"; group: "petalos"
        width: parent.width; y: -30
        emitRate: 2; lifeSpan: 16000
        velocity: AngleDirection { angle: 90; angleVariation: 20; magnitude: 55; magnitudeVariation: 25 }
        size: 18; sizeVariation: 10
    }
    Wander { system: ps; enabled: lw.scene === "petalos"; groups: ["petalos"]; xVariance: 60; pace: 40 }
    ItemParticle {
        system: ps; visible: lw.scene === "petalos"; groups: ["petalos"]
        // pétalo curvo con degradado; «da vueltas» en el aire: gira y a la vez se estrecha y ensancha (como si se
        // volteara), cada uno a su ritmo
        delegate: Image {
            id: pet
            source: "qrc:/icons/petalo.svg"
            width: 10 + Math.random() * 8; height: width * 2.6
            sourceSize: Qt.size(24, 64)
            opacity: 0.75 + Math.random() * 0.25
            rotation: Math.random() * 360
            RotationAnimation on rotation { from: pet.rotation; to: pet.rotation + (Math.random() < 0.5 ? 360 : -360)
                                             duration: 6000 + Math.random() * 6000; loops: Animation.Infinite; running: !lw.paused }
            transform: Scale {
                origin.x: pet.width / 2
                SequentialAnimation on xScale { loops: Animation.Infinite; running: !lw.paused
                    NumberAnimation { to: 0.15; duration: 700 + Math.random() * 900; easing.type: Easing.InOutSine }
                    NumberAnimation { to: 1; duration: 700 + Math.random() * 900; easing.type: Easing.InOutSine } }
            }
        }
        fade: true
    }

    // ---- luciérnagas: puntos de luz cálida que flotan ----
    Emitter {
        system: ps; enabled: lw.scene === "luciernagas"; group: "luciernagas"
        anchors.fill: parent
        emitRate: 4; lifeSpan: 9000; lifeSpanVariation: 3000
        velocity: AngleDirection { angleVariation: 360; magnitude: 12; magnitudeVariation: 8 }
        size: 6; sizeVariation: 4; endSize: 2
    }
    ItemParticle {
        system: ps; visible: lw.scene === "luciernagas"; groups: ["luciernagas"]
        delegate: Rectangle {
            width: 8; height: 8; radius: 4
            color: "#ffe9a8"
            Rectangle { anchors.centerIn: parent; width: 24; height: 24; radius: 12; color: "#33ffd27a" }
            SequentialAnimation on opacity { loops: Animation.Infinite; running: !lw.paused
                NumberAnimation { to: 0.2; duration: 900 + Math.random() * 900 }
                NumberAnimation { to: 1; duration: 900 + Math.random() * 900 } }
        }
        fade: true
    }

    // ---- lluvia ----
    Emitter {
        system: ps; enabled: lw.scene === "lluvia"; group: "lluvia"
        width: parent.width * 1.2; x: -parent.width * 0.1; y: -20
        emitRate: Th.soft ? 40 : 90; lifeSpan: 1600
        velocity: AngleDirection { angle: 100; angleVariation: 3; magnitude: 900; magnitudeVariation: 200 }
    }
    ItemParticle {
        system: ps; visible: lw.scene === "lluvia"; groups: ["lluvia"]
        delegate: Rectangle { width: 1.5; height: 22; color: "#55d8e6ff"; rotation: 10 }
    }

    // ---- sakura y hojas de otoño: caen meciéndose, como los pétalos ----
    Emitter {
        system: ps; enabled: lw.scene === "sakura" || lw.scene === "hojas"; group: "caida"
        width: parent.width; y: -40
        emitRate: lw.scene === "hojas" ? 1.5 : 3; lifeSpan: 18000
        velocity: AngleDirection { angle: 95; angleVariation: 25; magnitude: lw.scene === "hojas" ? 70 : 45; magnitudeVariation: 25 }
    }
    Wander { system: ps; enabled: lw.scene === "sakura" || lw.scene === "hojas"; groups: ["caida"]; xVariance: 90; pace: 50 }
    ItemParticle {
        system: ps; visible: lw.scene === "sakura" || lw.scene === "hojas"; groups: ["caida"]
        delegate: Image {
            id: fl
            readonly property bool hoja: lw.scene === "hojas"
            source: hoja ? "qrc:/icons/hoja-otono.svg" : "qrc:/icons/petalo-sakura.svg"
            width: hoja ? 22 + Math.random() * 16 : 12 + Math.random() * 8; height: width * (hoja ? 1 : 1.1)
            sourceSize: Qt.size(48, 48)
            opacity: 0.8 + Math.random() * 0.2
            rotation: Math.random() * 360
            RotationAnimation on rotation { from: fl.rotation; to: fl.rotation + (Math.random() < 0.5 ? 360 : -360)
                                             duration: 5000 + Math.random() * 7000; loops: Animation.Infinite; running: !lw.paused }
            transform: Scale {
                origin.x: fl.width / 2
                SequentialAnimation on xScale { loops: Animation.Infinite; running: !lw.paused
                    NumberAnimation { to: 0.25; duration: 900 + Math.random() * 900; easing.type: Easing.InOutSine }
                    NumberAnimation { to: 1; duration: 900 + Math.random() * 900; easing.type: Easing.InOutSine } }
            }
        }
        fade: true
    }

    // ---- nieve: copos de tamaños distintos, los grandes caen más rápido (parecen más cerca) ----
    Emitter {
        system: ps; enabled: lw.scene === "nieve"; group: "nieve"
        width: parent.width * 1.1; x: -parent.width * 0.05; y: -20
        emitRate: Th.soft ? 14 : 28; lifeSpan: 20000
        velocity: AngleDirection { angle: 92; angleVariation: 12; magnitude: 40; magnitudeVariation: 30 }
    }
    Wander { system: ps; enabled: lw.scene === "nieve"; groups: ["nieve"]; xVariance: 40; pace: 30 }
    ItemParticle {
        system: ps; visible: lw.scene === "nieve"; groups: ["nieve"]
        delegate: Rectangle {
            readonly property real s: 2 + Math.random() * Math.random() * 7
            width: s; height: s; radius: s / 2
            color: "white"; opacity: 0.45 + s / 14
        }
        fade: true
    }

    // ---- burbujas de luz: círculos suaves que suben despacio (bokeh) ----
    Emitter {
        system: ps; enabled: lw.scene === "burbujas"; group: "burbujas"
        width: parent.width; y: parent.height + 40
        emitRate: 1.2; lifeSpan: 22000
        velocity: AngleDirection { angle: -90; angleVariation: 15; magnitude: 22; magnitudeVariation: 12 }
    }
    Wander { system: ps; enabled: lw.scene === "burbujas"; groups: ["burbujas"]; xVariance: 30; pace: 12 }
    ItemParticle {
        system: ps; visible: lw.scene === "burbujas"; groups: ["burbujas"]
        delegate: Rectangle {
            readonly property real s: 30 + Math.random() * 90
            width: s; height: s; radius: s / 2
            color: Qt.rgba(1, 0.55 + Math.random() * 0.35, 0.5 + Math.random() * 0.4, 0.10 + Math.random() * 0.10)
            border.color: Qt.rgba(1, 1, 1, 0.18); border.width: 1
        }
        fade: true
    }

    // ---- cielo estrellado: estrellas que titilan y, de vez en cuando, una estrella fugaz ----
    Repeater {
        model: lw.scene === "estrellas" ? 90 : 0
        Rectangle {
            readonly property real s: 1 + Math.random() * 2.2
            x: Math.random() * lw.width; y: Math.random() * lw.height * 0.75
            width: s; height: s; radius: s / 2; color: "white"
            SequentialAnimation on opacity { loops: Animation.Infinite; running: !lw.paused
                NumberAnimation { to: 0.15; duration: 1200 + Math.random() * 2600; easing.type: Easing.InOutSine }
                NumberAnimation { to: 1; duration: 1200 + Math.random() * 2600; easing.type: Easing.InOutSine } }
        }
    }
    Rectangle {
        id: fugaz
        visible: lw.scene === "estrellas"
        width: 160; height: 2; radius: 1; rotation: 18; opacity: 0
        gradient: Gradient { orientation: Gradient.Horizontal
            GradientStop { position: 0; color: "transparent" }
            GradientStop { position: 1; color: "white" } }
        SequentialAnimation {
            running: lw.scene === "estrellas" && !lw.paused; loops: Animation.Infinite
            PauseAnimation { duration: 4000 + Math.random() * 6000 }
            ScriptAction { script: { fugaz.x = Math.random() * lw.width * 0.6; fugaz.y = Math.random() * lw.height * 0.35 } }
            ParallelAnimation {
                NumberAnimation { target: fugaz; property: "x"; to: fugaz.x + 420; duration: 900; easing.type: Easing.OutQuad }
                NumberAnimation { target: fugaz; property: "y"; to: fugaz.y + 140; duration: 900; easing.type: Easing.OutQuad }
                SequentialAnimation {
                    NumberAnimation { target: fugaz; property: "opacity"; to: 1; duration: 150 }
                    NumberAnimation { target: fugaz; property: "opacity"; to: 0; duration: 750 } }
            }
        }
    }

    // ---- aurora boreal: cortinas de luz verde y violeta que ondulan despacio sobre la parte de arriba ----
    Repeater {
        model: lw.scene === "aurora" ? 3 : 0
        Rectangle {
            id: band
            readonly property var cols: [["#3dffb0", "#1ec8ff"], ["#7c5cff", "#3dffb0"], ["#ff5fa2", "#7c5cff"]][index]
            width: lw.width * 1.6; height: lw.height * 0.45
            x: -lw.width * 0.3; y: lw.height * (0.02 + index * 0.07)
            opacity: 0.22 - index * 0.04
            rotation: -6 + index * 5
            gradient: Gradient {
                GradientStop { position: 0; color: "transparent" }
                GradientStop { position: 0.45; color: band.cols[0] }
                GradientStop { position: 0.7; color: band.cols[1] }
                GradientStop { position: 1; color: "transparent" }
            }
            SequentialAnimation on x { loops: Animation.Infinite; running: !lw.paused
                NumberAnimation { to: -lw.width * 0.05; duration: 14000 + index * 5000; easing.type: Easing.InOutSine }
                NumberAnimation { to: -lw.width * 0.5; duration: 14000 + index * 5000; easing.type: Easing.InOutSine } }
            SequentialAnimation on opacity { loops: Animation.Infinite; running: !lw.paused
                NumberAnimation { to: 0.3 - index * 0.05; duration: 6000 + index * 2000; easing.type: Easing.InOutSine }
                NumberAnimation { to: 0.12; duration: 6000 + index * 2000; easing.type: Easing.InOutSine } }
        }
    }
}
