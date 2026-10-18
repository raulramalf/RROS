// Teclado y atajos: distribución, repetición y atajos propios
import QtQuick
import QtQuick.Layouts
import "."

RPage {
    id: p
    title: "Teclado y atajos"
    subtitle: "Distribución, repetición de teclas y tus atajos"
    icon: "qrc:/icons/sec-teclado.svg"

    function dyn(fam) { return Ui.st ? Ui.st.dynamic[fam] : [] }
    function addKey(fam, title, help, ph) {
        ask.ask(title, help, [{ id: "id", label: "Nombre corto (a-z, 0-9)", placeholder: "navegador" }, { id: "v", label: "Valor", placeholder: ph }], "Añadir",
                function (v) { Ui.setKey(fam + "." + v.id.toLowerCase().replace(/[^a-z0-9_-]/g, ""), v.v) })
    }

    Ask { id: ask }

    Card {
        SRow {
            title: "Distribución del teclado"
            subtitle: "La misma en el escritorio, la consola y la pantalla de inicio de sesión"
            Combo {
                options: [["es", "Español (España)"], ["latam", "Español (Latinoamérica)"], ["us", "Inglés (EE. UU.)"], ["gb", "Inglés (Reino Unido)"],
                          ["us(intl)", "Inglés internacional"], ["pt", "Portugués"], ["br", "Portugués (Brasil)"], ["fr", "Francés"], ["de", "Alemán"],
                          ["it", "Italiano"], ["es(cat)", "Catalán"]]
                value: Ui.v("input.keymap")
                onPicked: function (v) { Ui.setKey("input.keymap", v.replace(/\(.*\)/, "")) }
            }
        }
        KeyRow { k: "input.repeat_delay_ms"; title: "Espera antes de repetir"; unit: " ms"; step: 50 }
        KeyRow { k: "input.repeat_rate"; title: "Repeticiones por segundo" }
        KeyRow { k: "input.numlock_on_boot"; title: "Bloq Num al iniciar"; separator: false }
    }

    Section { text: "Atajos del sistema" }
    Card {
        Repeater {
            model: [["Super", "Launchpad: todas las aplicaciones"], ["Alt + Espacio", "Buscar aplicaciones y archivos"], ["Super + E", "Archivos"],
                    ["Super + T / Ctrl + Alt + T", "Terminal"], ["Alt + F4", "Cerrar la ventana"], ["Super + L", "Bloquear la pantalla"],
                    ["Super + W", "Ver todas las ventanas"], ["Impr Pant", "Captura de pantalla"]]
            SRow { title: modelData[1]; Txt { text: modelData[0]; color: Ui.muted; font.family: "JetBrains Mono" } }
        }
        SRow {
            title: "Todos los atajos de KDE"
            separator: false
            Btn { text: "Abrir…"; onClicked: Sys.detached(["systemsettings", "kcm_keys"]) }
        }
    }

    Section { text: "Mis atajos" }
    Card {
        Repeater {
            model: p.dyn("keybind")
            SRow {
                title: modelData.id
                subtitle: modelData.value
                Btn { text: "Quitar"; kind: "danger"; onClicked: Ui.unsetKey("keybind." + modelData.id) }
            }
        }
        SRow {
            title: "Añadir un atajo"
            subtitle: "Combinación y programa, p. ej. «Mod+Shift+b firefox». Funciona desde el próximo inicio de sesión."
            separator: false
            Btn { text: "Añadir…"; kind: "primary"; onClicked: p.addKey("keybind", "Nuevo atajo", "Mod es la tecla Super (la del logotipo)", "Mod+Shift+b firefox") }
        }
    }

    Section { text: "Reglas de ventanas" }
    Card {
        Repeater {
            model: p.dyn("rule")
            SRow { title: modelData.id; subtitle: modelData.value; Btn { text: "Quitar"; kind: "danger"; onClicked: Ui.unsetKey("rule." + modelData.id) } }
        }
        SRow {
            title: "Añadir una regla"
            subtitle: "Que una aplicación se abra siempre encima, maximizada, a pantalla completa, sin bordes o transparente: «app=mpv above»"
            separator: false
            Btn { text: "Añadir…"; onClicked: p.addKey("rule", "Nueva regla", "Acciones: above, maximized, fullscreen, noborder, skiptaskbar, minimized, opacity 80", "app=mpv above") }
        }
    }

    Section { text: "Al iniciar sesión" }
    Card {
        Repeater {
            model: p.dyn("autostart")
            SRow { title: modelData.id; subtitle: modelData.value; Btn { text: "Quitar"; kind: "danger"; onClicked: Ui.unsetKey("autostart." + modelData.id) } }
        }
        SRow {
            title: "Abrir un programa al iniciar"
            separator: false
            Btn { text: "Añadir…"; onClicked: p.addKey("autostart", "Programa de inicio", "El comando que se ejecuta al entrar", "syncthing -no-browser") }
        }
    }
}
