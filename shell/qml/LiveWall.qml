// Escenas animadas sobre el fondo: pétalos de lirio que caen, luciérnagas o lluvia. Con partículas de Qt Quick:
// pocas y sencillas, para que no cuesten CPU. Se paran con la batería si así se elige.
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
        system: ps; enabled: lw.scene === "petalos"
        width: parent.width; y: -30
        emitRate: 2; lifeSpan: 16000
        velocity: AngleDirection { angle: 90; angleVariation: 20; magnitude: 55; magnitudeVariation: 25 }
        size: 18; sizeVariation: 10
    }
    Wander { system: ps; enabled: lw.scene === "petalos"; xVariance: 60; pace: 40 }
    ItemParticle {
        system: ps; visible: lw.scene === "petalos"
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
        system: ps; enabled: lw.scene === "luciernagas"
        anchors.fill: parent
        emitRate: 4; lifeSpan: 9000; lifeSpanVariation: 3000
        velocity: AngleDirection { angleVariation: 360; magnitude: 12; magnitudeVariation: 8 }
        size: 6; sizeVariation: 4; endSize: 2
    }
    ItemParticle {
        system: ps; visible: lw.scene === "luciernagas"
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
        system: ps; enabled: lw.scene === "lluvia"
        width: parent.width * 1.2; x: -parent.width * 0.1; y: -20
        emitRate: 90; lifeSpan: 1600
        velocity: AngleDirection { angle: 100; angleVariation: 3; magnitude: 900; magnitudeVariation: 200 }
    }
    ItemParticle {
        system: ps; visible: lw.scene === "lluvia"
        delegate: Rectangle { width: 1.5; height: 22; color: "#55d8e6ff"; rotation: 10 }
    }
}
