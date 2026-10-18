// Estado del sistema para la barra y el centro de control: red, sonido, batería, Bluetooth y brillo.
pragma Singleton
import QtQuick

QtObject {
    id: s
    property string netType: ""      // wifi, ethernet, ""
    property string netName: ""
    property int wifiSignal: 0
    property bool wifiOn: true
    property int volume: 50
    property bool muted: false
    property int battery: -1
    property bool charging: false
    property bool btOn: false
    property bool btPresent: false
    property int brightness: -1
    property bool nightLight: Th.v("ui.night_light", "off") !== "off"

    function poll() {
        Sh.run(["nmcli", "-t", "-f", "TYPE,STATE,CONNECTION", "device"], function (r) {
            var t = "", n = ""
            r.out.split("\n").forEach(function (l) {
                var f = l.split(":")
                if (f[1] === "connected" && (f[0] === "wifi" || (f[0] === "ethernet" && t === ""))) { t = f[0]; n = f.slice(2).join(":") }
            })
            s.netType = t; s.netName = n
        })
        Sh.run(["nmcli", "-t", "-f", "IN-USE,SIGNAL", "device", "wifi", "list", "--rescan", "no"], function (r) {
            r.out.split("\n").forEach(function (l) { if (l.indexOf("*:") === 0) s.wifiSignal = parseInt(l.slice(2)) || 0 })
        })
        Sh.run(["nmcli", "-t", "radio", "wifi"], function (r) { s.wifiOn = r.out.trim() !== "disabled" })
        Sh.run(["wpctl", "get-volume", "@DEFAULT_AUDIO_SINK@"], function (r) {
            var m = r.out.match(/([0-9.]+)/)
            if (m) s.volume = Math.round(parseFloat(m[1]) * 100)
            s.muted = r.out.indexOf("MUTED") >= 0
        })
        Sh.run(["sh", "-c", "for b in /sys/class/power_supply/BAT*; do [ -d \"$b\" ] && cat \"$b/capacity\" \"$b/status\"; break; done"], function (r) {
            var l = r.out.trim().split("\n")
            s.battery = l.length >= 2 ? parseInt(l[0]) : -1
            s.charging = l.length >= 2 && l[1] !== "Discharging"
        })
        Sh.run(["bluetoothctl", "show"], function (r) { s.btPresent = r.code === 0 && r.out.indexOf("Controller") >= 0; s.btOn = /Powered: yes/.test(r.out) })
        Sh.run(["brightnessctl", "-m"], function (r) { var f = r.out.split(","); s.brightness = r.code === 0 && f.length > 3 ? parseInt(f[3]) : -1 })
    }
    function setVolume(v) {
        v = Math.max(0, Math.min(150, Math.round(v)))
        volume = v
        Sh.run(["wpctl", "set-volume", "@DEFAULT_AUDIO_SINK@", (v / 100).toFixed(2)], function () {})
    }
    function toggleMute() { muted = !muted; Sh.run(["wpctl", "set-mute", "@DEFAULT_AUDIO_SINK@", "toggle"], function () {}) }
    function setBrightness(v) { brightness = v; Sh.run(["brightnessctl", "set", Math.round(v) + "%"], function () {}) }
    function setWifi(on) { wifiOn = on; Sh.run(["nmcli", "radio", "wifi", on ? "on" : "off"], function () { s.poll() }) }
    function setBt(on) { btOn = on; Sh.run(["bluetoothctl", "power", on ? "on" : "off"], function () { s.poll() }) }

    property Timer timer: Timer { interval: 5000; running: true; repeat: true; triggeredOnStart: true; onTriggered: s.poll() }
    // teclas de volumen
    property Connections keys: Connections {
        target: Sh
        function onVolumeKey(step) {
            if (step === 0) s.toggleMute()
            else s.setVolume(s.volume + step * Th.n("audio.volume_step", 5))
            Sh.showOsd(s.muted ? "audio-volume-muted" : "audio-volume-high", s.muted ? 0 : s.volume, "Volumen")
        }
    }
}
