// Fecha, hora e idioma
import QtQuick
import QtQuick.Layouts
import "."

RPage {
    id: p
    title: "Fecha, hora e idioma"
    subtitle: "Zona horaria, formato del reloj e idioma del sistema"
    icon: "qrc:/icons/sec-fecha.svg"
    property var zones: []
    property string now: ""
    onShown: {
        Ui.run(["timedatectl", "list-timezones"], function (r) { zones = Ui.lines(r.out).map(function (z) { return [z, z.replace(/_/g, " ")] }) })
    }
    Timer { interval: 1000; running: p.visible; repeat: true; triggeredOnStart: true
            onTriggered: p.now = Qt.formatDateTime(new Date(), "dddd d 'de' MMMM yyyy · " + (Ui.b("ui.clock_24h") ? "HH:mm:ss" : "h:mm:ss AP")) }

    Card {
        SRow { title: p.now; subtitle: "Se ajusta sola por Internet" }
        SRow {
            title: "Zona horaria"
            Combo { implicitWidth: 260; options: p.zones; value: Ui.v("locale.timezone"); onPicked: function (v) { Ui.setKey("locale.timezone", v) } }
        }
        KeyRow { k: "ui.clock_24h"; title: "Reloj de 24 horas"; separator: false }
    }
    Section { text: "Idioma y región" }
    Card {
        SRow {
            title: "Idioma del sistema"
            subtitle: "Menús de las aplicaciones. Se aplica al volver a iniciar sesión."
            Combo {
                options: [["es_ES.UTF-8", "Español (España)"], ["es_MX.UTF-8", "Español (México)"], ["es_AR.UTF-8", "Español (Argentina)"],
                          ["ca_ES.UTF-8", "Català"], ["gl_ES.UTF-8", "Galego"], ["eu_ES.UTF-8", "Euskara"], ["en_US.UTF-8", "English (US)"],
                          ["en_GB.UTF-8", "English (UK)"], ["pt_PT.UTF-8", "Português"], ["fr_FR.UTF-8", "Français"], ["de_DE.UTF-8", "Deutsch"]]
                value: Ui.v("locale.language")
                onPicked: function (v) { Ui.setKey("locale.language", v) }
            }
        }
        KeyRow { k: "locale.units"; title: "Unidades"; labels: { "metric": "Métricas", "imperial": "Imperiales" }; separator: false }
    }
}
