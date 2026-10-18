// General: este equipo, la versión de RROS y qué edición le va mejor
import QtQuick
import QtQuick.Layouts
import "."

RPage {
    id: p
    title: "General"
    subtitle: "Tu equipo y la versión de RROS"
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
            Image { source: "qrc:/icons/app-rros.svg"; sourceSize: Qt.size(96, 96) }
            ColumnLayout {
                spacing: 4
                Txt { text: "RROS"; color: "#ffffff"; font.pixelSize: 30; font.weight: Font.Black }
                Txt { text: Ui.st ? "Versión " + Ui.st.version + " «" + Ui.st.codename + "» · edición " + Ui.st.profile.charAt(0).toUpperCase() + Ui.st.profile.slice(1) : ""; color: "#e8ecff" }
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

    Section { text: "Ediciones de RROS" }
    Note {
        text: Ui.st && Ui.st.hw ? "Para este equipo (" + Math.round(Ui.st.hw.ram_mb / 1024) + " GB de RAM, " + Ui.st.hw.cpus +
              " hilos) se recomienda la edición " + Ui.st.hw.recommended.charAt(0).toUpperCase() + Ui.st.hw.recommended.slice(1) + "." : ""
        visible: text !== ""
    }
    Card {
        Repeater {
            model: [
                ["lite", "Lite", "2–4 GB de RAM · 2 núcleos de 64 bits · 10 GB de disco",
                 "Sin efectos: todo aparece al instante. Unos 700 MB de RAM y casi nada de procesador en reposo."],
                ["standard", "Standard", "4–8 GB de RAM · 4 hilos · 14 GB de disco",
                 "Animaciones rápidas y apps de oficina. Alrededor de 1 GB de RAM en reposo."],
                ["pro", "Pro", "8 GB de RAM · 4 núcleos y GPU · 20 GB de disco",
                 "Todos los efectos: desenfoque, lámpara mágica, transparencias y el dock en 3D."]
            ]
            SRow {
                title: modelData[1] + (Ui.st && Ui.st.profile === modelData[0] ? "  ·  la tuya" : "")
                subtitle: modelData[2] + "\n" + modelData[3]
                separator: index < 2
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
