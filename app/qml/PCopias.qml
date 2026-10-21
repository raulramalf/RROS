// Copias y restauración: la vuelta atrás del sistema (instantáneas Btrfs antes de cada actualización, también en el
// menú de arranque) y las copias de seguridad de tu carpeta personal a un disco o a la nube. Todo con rros-copias.
import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import "."

RPage {
    id: p
    title: "Copias y restauración"
    subtitle: "Vuelve atrás si una actualización sale mal y guarda tus archivos en otro sitio"
    icon: "qrc:/icons/sec-avanzado.svg"
    property var st: null
    property var snaps: []
    property var versions: []
    property bool working: false
    property string log: ""
    function json(argv, cb) { Sys.run(argv, function (r) { var v = null; try { v = JSON.parse(r.out) } catch (e) {} cb(v) }) }
    function refresh() {
        json(["rros-copias", "estado", "--json"], function (v) { p.st = v })
        json(["rros-copias", "instantaneas", "--json"], function (v) { p.snaps = v || [] })
        json(["rros-copias", "versiones", "--json"], function (v) { p.versions = v || [] })
    }
    function root(args, ok) {
        Sys.run(["pkexec", "rros-copias"].concat(args), function (r) { Ui.toast(r.code === 0 ? ok : (r.err || "No se pudo"), r.code !== 0); p.refresh() })
    }
    onShown: refresh()

    Section { text: "Vuelta atrás del sistema" }
    Card {
        SRow {
            title: p.st && p.st.instantaneas ? "Activada" : "No disponible en este equipo"
            subtitle: p.st && p.st.instantaneas
                ? "Antes de cada actualización se guarda el estado del sistema. Si algo falla, vuelve atrás aquí o eligiendo «Lily snapshots» en el menú de arranque. Tus archivos personales no cambian."
                : "Necesita Lily instalado en Btrfs (el instalador lo hace solo). En el sistema en vivo no hay instantáneas."
            Btn { visible: p.st && p.st.instantaneas; text: "Crear una ahora"; onClicked: p.root(["crear", "Manual"], "Instantánea creada") }
        }
        Repeater {
            model: p.snaps.slice().reverse()
            SRow {
                title: modelData.desc || ("Instantánea " + modelData.n)
                subtitle: modelData.fecha + (modelData.tipo === "pre" ? " · antes de actualizar" : modelData.tipo === "post" ? " · después de actualizar" : "")
                separator: index < p.snaps.length - 1
                RowLayout {
                    Btn { text: "Volver a esta"; onClicked: confirm.ask("¿Volver el sistema a la instantánea " + modelData.n + "?",
                        "Programas y ajustes del sistema volverán a como estaban. Tus documentos no cambian. Se reiniciará el equipo.", [], "Volver y reiniciar",
                        function () { var n = modelData.n; Sys.run(["pkexec", "rros-copias", "restaurar", String(n)], function (r) {
                            if (r.code === 0) Sys.detached(["systemctl", "reboot"]); else Ui.toast(r.err || "No se pudo", true) }) }) }
                    Btn { text: "Borrar"; kind: "danger"; onClicked: p.root(["borrar", String(modelData.n)], "Borrada") }
                }
            }
        }
    }

    Section { text: "Copia de seguridad de tu carpeta personal" }
    Card {
        SRow {
            title: p.st && p.st.destino ? (p.st.nube ? "En la nube: " : "En: ") + p.st.destino : "Sin destino"
            subtitle: p.st && p.st.destino ? (p.st.conectado ? "Copia diaria automática. Última: " + (p.st.ultima ? p.st.ultima.slice(0, 16).replace("T", " ") : "todavía ninguna")
                                                            : "El disco no está conectado: la copia se hará cuando lo conectes.")
                     : "Elige un disco externo (cada copia guarda solo lo que cambió, como Time Machine) o una carpeta en la nube."
            RowLayout {
                Btn { text: "Disco o carpeta…"; onClicked: folder.open() }
                Btn { text: "Copiar ahora"; kind: "primary"; visible: p.st && p.st.destino !== ""; busy: p.working
                      onClicked: { p.working = true; Sys.stream(["rros-copias", "copiar"], function (l) { p.log = l },
                                   function (c) { p.working = false; Ui.toast(c === 0 ? "Copia terminada" : "No se pudo copiar: " + p.log, c !== 0); p.refresh() }) } }
            }
        }
        SRow {
            title: "En la nube"
            subtitle: "Google Drive, OneDrive, Dropbox, Nextcloud… Configura la cuenta con rclone (rclone config) y escribe aquí «nombre:carpeta»."
            Field { id: cloud; implicitWidth: 220; placeholderText: "drive:Copias"; onAccepted: Sys.run(["rros-copias", "destino", text], function () { p.refresh() }) }
        }
        SRow {
            visible: p.working; title: "Copiando…"; subtitle: p.log
        }
        Repeater {
            model: p.versions.slice().reverse().slice(0, 10)
            SRow {
                title: "Copia del " + modelData.nombre.replace("_", " a las ").replace(/(\d\d)(\d\d)$/, "$1:$2")
                separator: index < Math.min(10, p.versions.length) - 1
                Btn { text: "Abrir"; onClicked: Sys.detached(["dolphin", modelData.ruta]) }
            }
        }
    }
    FolderDialog { id: folder; title: "Disco o carpeta para las copias"; onAccepted: Sys.run(["rros-copias", "destino", decodeURIComponent(String(selectedFolder).replace("file://", ""))], function () { p.refresh() }) }
    Ask { id: confirm }
}
