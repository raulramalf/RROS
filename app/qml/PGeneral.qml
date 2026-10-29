// General: este equipo, la versión de Lily y el nivel al que se ajusta (automático según el hardware, o fijo)
import QtQuick
import QtQuick.Layouts
import "."

RPage {
    id: p
    title: "General"
    subtitle: "Tu equipo y la versión de Lily"
    icon: "qrc:/icons/sec-general.svg"
    property string cpu: ""
    property int cores: 0
    property string mem: ""
    property string disk: ""
    property string kernel: ""
    property string gpu: ""
    property string up: ""

    function refresh() {
        var ci = Sys.readFile("/proc/cpuinfo")
        var m = ci.match(/model name\s*:\s*(.*)/)
        cpu = m ? m[1].replace(/\s+/g, " ") : "Procesador"
        cores = (ci.match(/^processor\s*:/mg) || []).length
        var mi = Sys.readFile("/proc/meminfo")
        var tot = parseInt((mi.match(/MemTotal:\s*(\d+)/) || [0, 0])[1]) * 1024
        var av = parseInt((mi.match(/MemAvailable:\s*(\d+)/) || [0, 0])[1]) * 1024
        mem = Ui.human(tot) + " · en uso " + Ui.human(tot - av)
        kernel = Sys.readFile("/proc/sys/kernel/osrelease").trim()
        var u = parseFloat(Sys.readFile("/proc/uptime"))
        up = Math.floor(u / 3600) + " h " + Math.floor(u % 3600 / 60) + " min"
        Ui.run(["df", "-B1", "--output=size,used", "/"], function (r) {
            var l = Ui.lines(r.out)
            if (l.length > 1) { var f = l[1].trim().split(/\s+/); disk = Ui.human(+f[0]) + " · en uso " + Ui.human(+f[1]) }
        })
        Ui.run(["sh", "-c", "for d in /sys/class/drm/card*/device; do cat $d/vendor $d/device 2>/dev/null; readlink $d/driver; done"], function (r) {
            var drv = (r.out.match(/drivers\/(\S+)/) || [0, ""])[1]
            var names = { "i915": "Intel", "xe": "Intel", "amdgpu": "AMD Radeon", "radeon": "AMD Radeon", "nouveau": "NVIDIA (libre)",
                          "nvidia": "NVIDIA", "vmwgfx": "VMware / VirtualBox", "vboxvideo": "VirtualBox", "virtio-pci": "Virtio (QEMU)",
                          "virtio_gpu": "Virtio (QEMU)", "qxl": "QXL (QEMU)", "bochs-drm": "QEMU" }
            gpu = drv ? (names[drv] || drv) : "Sin aceleración"
        })
    }
    onShown: refresh()

    // cabecera
    Rectangle {
        Layout.fillWidth: true
        implicitHeight: 150
        radius: 16
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0; color: Qt.darker(Ui.accent, 1.6) }
            GradientStop { position: 1; color: Qt.darker(Ui.pal.cyan, 1.9) }
        }
        RowLayout {
            anchors { fill: parent; margins: 24 }
            spacing: 22
            Image { source: "qrc:/icons/app-rros.png"; sourceSize: Qt.size(96, 96) }
            ColumnLayout {
                spacing: 4
                Txt { text: "Lily"; color: "#ffffff"; font.pixelSize: 30; font.weight: Font.Black }
                Txt { text: Ui.st ? "Versión " + Ui.st.version + " «" + Ui.st.codename + "» · nivel " + p.tierName(Ui.st.profile) + (p.mode === "auto" ? " (automático)" : "") : ""; color: "#e8ecff" }
                Txt { text: Sys.live ? "Sistema en vivo (desde la ISO o la memoria USB)" : "Instalado · encendido hace " + p.up; color: "#c9d1f5"; font.pixelSize: Ui.fs - 1 }
                Layout.fillWidth: true
            }
        }
    }

    Section { text: "Este equipo" }
    Card {
        SRow { title: "Procesador"; subtitle: p.cpu + (p.cores ? " · " + p.cores + " hilos" : "") }
        SRow { title: "Memoria"; subtitle: p.mem }
        SRow { title: "Gráfica"; subtitle: p.gpu }
        SRow { title: "Disco del sistema"; subtitle: p.disk }
        SRow { title: "Kernel Linux"; subtitle: p.kernel; separator: false }
    }
    Card {
        KeyRow { k: "net.hostname"; title: "Nombre del equipo"; subtitle: "Cómo lo ven los demás en la red"; separator: false; placeholder: Sys.readFile("/proc/sys/kernel/hostname").trim() }
    }

    // una sola Lily que se ajusta al equipo: rros-nivel elige en cada arranque ligero, equilibrado o máximo
    readonly property var tiers: ({ lite: "ligero", standard: "equilibrado", pro: "máximo" })
    function tierName(t) { return tiers[t] || t }
    property string mode: { var m = Sys.readFile("/etc/rros/nivel").trim(); return m === "lite" || m === "standard" || m === "pro" ? m : "auto" }
    function setMode(m) {
        Sys.run(["pkexec", "rros-nivel", "fijar", m], function (r) {
            if (r.code === 0) { p.mode = Sys.readFile("/etc/rros/nivel").trim() || "auto"; Ui.run(["rros-theme", "apply", "--quiet"]); Ui.reload()
                                Ui.toast("Lily se ajusta ahora al nivel " + p.tierName(Sys.readFile("/etc/rros/profile").trim()), false) }
            else Ui.toast(r.code === 126 || r.code === 127 ? "Hace falta la contraseña de un administrador" : "No se pudo cambiar el nivel", true)
        })
    }
    Section { text: "Ajuste a este equipo" }
    Note {
        text: Ui.st && Ui.st.hw ? "Lily es una sola versión que se adapta a cada equipo. Con " + Math.round(Ui.st.hw.ram_mb / 1024) + " GB de RAM y " +
              Ui.st.hw.cpus + " hilos le corresponde el nivel " + p.tierName(Ui.st.hw.recommended) +
              ". En cualquier nivel, Lily en reposo usa como mucho 2 GB de RAM y ocupa menos de 15 GB de disco." : ""
        visible: text !== ""
    }
    Card {
        Repeater {
            model: [
                ["auto", "Automático (recomendado)", "Al arrancar, Lily mira la RAM, el procesador y la gráfica y elige el nivel. Si cambias de equipo o amplías la memoria, se ajusta solo."],
                ["lite", "Ligero", "Sin efectos ni animaciones: todo aparece al instante. Para equipos de 2-3 GB o sin gráfica. Menos de 1,3 GB de RAM en reposo."],
                ["standard", "Equilibrado", "Animaciones rápidas, sin desenfoque. Para 3-6 GB de RAM. Menos de 1,5 GB de RAM en reposo."],
                ["pro", "Máximo", "Todos los efectos: desenfoque, transparencias y el dock en 3D. Para 6 GB o más, 4 hilos y gráfica. Menos de 2 GB en reposo."]
            ]
            SRow {
                title: modelData[1] + (p.mode === modelData[0] ? "  ·  elegido" : (p.mode === "auto" && Ui.st && Ui.st.profile === modelData[0] ? "  ·  el de ahora" : ""))
                subtitle: modelData[2]
                separator: index < 3
                clickable: !Sys.live && p.mode !== modelData[0]
                onClicked: p.setMode(modelData[0] === "auto" ? "automatico" : modelData[0])
            }
        }
    }

    Section { text: "Más" }
    Card {
        SRow {
            title: "Bienvenida"
            subtitle: "El asistente del primer inicio: aspecto, dock y escritorio"
            clickable: true
            onClicked: Sys.detached(["rros-ajustes", "--bienvenida"])
            Txt { text: "›"; color: Ui.muted; font.pixelSize: 20 }
        }
        SRow {
            title: "Monitor del sistema"
            subtitle: "Procesos, memoria y procesador en tiempo real"
            clickable: true
            separator: false
            onClicked: Sys.detached(["plasma-systemmonitor"])
            Txt { text: "›"; color: Ui.muted; font.pixelSize: 20 }
        }
    }
}
