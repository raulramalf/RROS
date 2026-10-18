import QtQuick
import QtQuick.Layouts
import "."

RPage {
    id: p
    title: "Accesibilidad"
    subtitle: "Ver, oír y manejar el equipo con más facilidad"
    icon: "qrc:/icons/sec-accesibilidad.svg"
    function a11y(what, label) { Ui.api("POST", "/api/a11y", { what: what }, function (r) { Ui.toast(r.ok ? label : (r.error || "No se pudo"), !r.ok) }) }

    Section { text: "Ahora" }
    Card {
        SRow { title: "Lector de pantalla"; subtitle: "Orca lee en voz alta lo que hay en pantalla (Super+Alt+S)"; Btn { text: "Activar o quitar"; onClicked: p.a11y("lector", "Lector de pantalla") } }
        SRow { title: "Teclado en pantalla"; subtitle: "Para escribir con el ratón o una pantalla táctil"; separator: false; Btn { text: "Activar o quitar"; onClicked: p.a11y("teclado", "Teclado en pantalla") } }
    }
    Section { text: "Siempre" }
    Card {
        KeyRow { k: "access.screen_reader"; title: "Lector de pantalla al iniciar" }
        KeyRow { k: "access.onscreen_keyboard"; title: "Teclado en pantalla al iniciar" }
        KeyRow { k: "ui.high_contrast"; title: "Alto contraste" }
        KeyRow { k: "ui.reduce_motion"; title: "Reducir el movimiento"; subtitle: "Sin animaciones" }
        KeyRow { k: "ui.font_size"; title: "Tamaño de la letra"; unit: " pt" }
        KeyRow { k: "ui.cursor_size"; title: "Tamaño del puntero"; unit: " px"; step: 4 }
        KeyRow { k: "ui.scale"; title: "Escala de las aplicaciones antiguas (X11)"; unit: " %"; step: 25; separator: false }
    }
}
