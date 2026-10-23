#!/usr/bin/env bash
# Prueba de integración de las herramientas con directorios temporales.
set -euo pipefail
cd "$(dirname "$0")/.."
B=$PWD/build
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
export RROS_PROFILE_DIR=$PWD/profiles
export RROS_SYSTEM_CONF=$T/etc/rros.toml
export RROS_USER_CONF=$T/home/rros.toml
export RROS_PROFILE_FILE=$T/etc/profile
fail() { echo "  FALLO: $*" >&2; exit 1; }
# grep -q que lee toda la entrada: con `pipefail`, un grep que sale al primer acierto mata al productor por SIGPIPE
gq() { local out; out=$(cat); printf '%s\n' "$out" | command grep -q "$@"; }
eq() { [ "$1" = "$2" ] || fail "esperado '$2', obtenido '$1'"; }

# perfil activo por defecto y escritura
eq "$("$B/rros-config" profile)" standard
RROS_PROFILE_FILE=$T/etc/profile "$B/rros-hwdetect" --apply >/dev/null
[ -s "$T/etc/profile" ] || fail "no se escribió el perfil"

# capas: perfil < sistema < usuario
eq "$("$B/rros-config" -p lite get ui.effects)" off
eq "$("$B/rros-config" -p pro get ui.effects)" advanced
"$B/rros-config" -p lite set --system ui.effects basic
eq "$("$B/rros-config" -p lite get ui.effects)" basic
"$B/rros-config" -p lite set ui.effects advanced
eq "$("$B/rros-config" -p lite get ui.effects)" advanced
"$B/rros-config" -p lite list | gq 'ui.effects = "advanced"  (usuario)' || fail "list/capa usuario"
"$B/rros-config" -p lite list | gq 'ui.blur = false  (perfil)' || fail "list/capa perfil"
"$B/rros-config" -p lite unset ui.effects
eq "$("$B/rros-config" -p lite get ui.effects)" basic

# valores de distintos tipos y persistencia con comillas correctas
"$B/rros-config" set ui.scale 125
"$B/rros-config" set ui.accent '#ff8800'
"$B/rros-config" set ui.animations true
eq "$("$B/rros-config" get ui.scale)" 125
eq "$("$B/rros-config" get ui.accent)" '#ff8800'
grep -q '^scale = 125$' "$T/home/rros.toml" || fail "entero sin comillas"
grep -q '^accent = "#ff8800"$' "$T/home/rros.toml" || fail "texto con comillas"
"$B/rros-config" validate

# validaciones que deben fallar
for args in "set ui.scale 9999" "set ui.scale abc" "set ui.effects loquesea" "set ui.animations si" \
            "set no.existe 1" "set profile.name x" "get no.existe" "bogus" "set ui.scale"; do
	# shellcheck disable=SC2086
	if "$B/rros-config" $args 2>/dev/null; then fail "debía fallar: $args"; fi
done

# validate detecta archivos malos
printf '[ui]\nscale = 5000\nraro = 1\n' > "$T/mal.toml"
if "$B/rros-config" validate "$T/mal.toml" 2>/dev/null; then fail "validate debía fallar"; fi
printf 'esto no es toml\n' > "$T/sint.toml"
if "$B/rros-config" validate "$T/sint.toml" 2>/dev/null; then fail "validate sintaxis"; fi

# -f lee un único archivo (usado por scripts de build)
eq "$("$B/rros-config" -f profiles/pro.toml get build.libc)" glibc
eq "$("$B/rros-config" -f profiles/lite.toml get ui.effects)" off

# budget
mkdir "$T/rootfs" && head -c 3000000 /dev/zero > "$T/rootfs/f"
"$B/rros-budget" disk "$T/rootfs" -p lite | gq 'status=ok' || fail "budget disk ok"
mkdir -p "$T/fakeroot/proc" && printf 'MemTotal: 4194304 kB\nMemAvailable: 1048576 kB\n' > "$T/fakeroot/proc/meminfo"
printf 'processor : 0\nprocessor : 1\nprocessor : 2\nprocessor : 3\n' > "$T/fakeroot/proc/cpuinfo"
RROS_ROOT=$T/fakeroot "$B/rros-budget" ram -p pro | gq 'status=ok' || fail "budget ram pro"
if RROS_ROOT=$T/fakeroot "$B/rros-budget" ram -p lite >/dev/null; then fail "budget ram lite debía superar"; fi

# hwdetect con raíz falsa y salidas
eq "$("$B/rros-hwdetect" --root "$T/fakeroot" --profile)" standard
"$B/rros-hwdetect" --json | gq '"profile"' || fail "json"
# --- escritorio propio: rros-theme, rros-fetch, rros ---
export XDG_CONFIG_HOME=$T/xdg
export XDG_DATA_HOME=$T/xdgdata XDG_STATE_HOME=$T/xdgstate
"$B/rros-theme" -p lite apply --quiet
for f in kwinrc kdeglobals kcminputrc kxkbrc powerdevilrc plasmarc; do
	[ -s "$XDG_CONFIG_HOME/$f" ] || fail "rros-theme no generó $f"
done
for f in konsole/Lily.profile konsole/Lily.colorscheme aurorae/themes/Lily/decoration.svg; do
	[ -s "$XDG_DATA_HOME/$f" ] || fail "rros-theme no generó $f"
done
ls "$XDG_DATA_HOME"/color-schemes/Lily-*.colors >/dev/null || fail "esquema de colores de Plasma"
grep -q 'blurEnabled=false' "$XDG_CONFIG_HOME/kwinrc" || fail "Lite sin desenfoque"
grep -q 'AnimationDurationFactor=0' "$XDG_CONFIG_HOME/kdeglobals" || fail "Lite sin animaciones"
"$B/rros-config" -p lite set ui.dock_style 3d
"$B/rros-theme" -p lite apply --quiet
grep -q 'name=rros-3d' "$XDG_CONFIG_HOME/plasmarc" || fail "el dock 3D no llega a Plasma"
# lo que el usuario cambie en las Preferencias de KDE se respeta
printf '\n[Mio]\nclave=1\n' >> "$XDG_CONFIG_HOME/kwinrc"
"$B/rros-theme" -p lite apply --quiet
grep -q '^clave=1' "$XDG_CONFIG_HOME/kwinrc" || fail "rros-theme borró ajustes propios de kwinrc"
# alternar oscuro/claro
"$B/rros-theme" -p lite toggle >/dev/null
eq "$("$B/rros-config" -p lite get ui.color_scheme)" light
grep -q 'Color=250,245,239' "$XDG_DATA_HOME/konsole/Lily.colorscheme" || fail "tema claro"
"$B/rros-theme" -p lite toggle >/dev/null
eq "$("$B/rros-config" -p lite get ui.color_scheme)" dark
# ficha y diagnóstico
"$B/rros-fetch" --plain | gq 'Perfil' || fail "rros-fetch"
"$B/rros-fetch" --plain | gq 'SO\|Sistema' || fail "rros-fetch sistema"
"$B/rros" version | gq 'Lirio' || fail "rros version"
# el resultado de la comprobación de snap depende del equipo (los runners de CI lo tienen)
doc_out=$("$B/rros" doctor) || true
case $doc_out in *snap*) ;; *) fail "rros doctor no informa de snap" ;; esac
case $doc_out in *'telemetría desactivada'*) ;; *) fail "rros doctor no informa de telemetría" ;; esac
"$B/rros" info --plain | gq 'Perfil' || fail "rros info"
if "$B/rros" bogus 2>/dev/null; then fail "rros bogus debía fallar"; fi
# --- rros-apply: el perfil cambia el sistema ---
mkdir -p "$T/sysroot"
"$B/rros-apply" -p lite --root "$T/sysroot" >/dev/null
grep -q 'compression-algorithm = lz4' "$T/sysroot/etc/systemd/zram-generator.conf" || fail "zram lite"
grep -q 'zram-size = ram \* 50 / 100' "$T/sysroot/etc/systemd/zram-generator.conf" || fail "zram tamaño"
grep -q 'vm.swappiness = 100' "$T/sysroot/etc/sysctl.d/90-rros-profile.conf" || fail "swappiness lite"
"$B/rros-apply" -p pro --root "$T/sysroot" >/dev/null
grep -q 'vm.swappiness = 60' "$T/sysroot/etc/sysctl.d/90-rros-profile.conf" || fail "swappiness pro"
grep -q 'zram-size = ram \* 25 / 100' "$T/sysroot/etc/systemd/zram-generator.conf" || fail "zram pro"
"$B/rros-config" -p pro set system.io_scheduler bfq
"$B/rros-apply" -p pro --root "$T/sysroot" >/dev/null
grep -q 'scheduler}="bfq"' "$T/sysroot/etc/udev/rules.d/60-rros-ioscheduler.rules" || fail "scheduler"
"$B/rros-config" -p pro set system.swap none
"$B/rros-apply" -p pro --root "$T/sysroot" >/dev/null
! grep -q '^\[zram0\]' "$T/sysroot/etc/systemd/zram-generator.conf" || fail "swap none"

# --- seguridad: rros-apply genera las directivas ---
"$B/rros-apply" -p standard --root "$T/sysroot" >/dev/null
grep -q 'policy drop' "$T/sysroot/etc/nftables.conf" || fail "cortafuegos"
grep -q 'kernel.kptr_restrict = 2' "$T/sysroot/etc/sysctl.d/80-rros-security.conf" || fail "kernel endurecido"
[ "$(stat -c %a "$T/sysroot/etc/sudoers.d/rros")" = 440 ] || fail "sudoers con permisos 0440"
grep -q 'timestamp_timeout=5' "$T/sysroot/etc/sudoers.d/rros" || fail "tiempo de sudo"
grep -q 'deny = 10' "$T/sysroot/etc/security/faillock.conf" || fail "bloqueo por intentos"
grep -q 'Unattended-Upgrade "1"' "$T/sysroot/etc/apt/apt.conf.d/20auto-upgrades" || fail "actualizaciones automáticas"
grep -q '^LANG=es_ES.UTF-8' "$T/sysroot/etc/default/locale" || fail "idioma del sistema"
grep -q '^LANGUAGE=es_ES:es' "$T/sysroot/etc/default/locale" || fail "LANGUAGE"
if "$B/rros-config" -p standard set locale.timezone '../../etc/shadow' 2>/dev/null; then fail "zona horaria con ruta aceptada"; fi
if "$B/rros-config" -p standard set locale.language 'es_ES.UTF-8;id' 2>/dev/null; then fail "idioma no válido aceptado"; fi
"$B/rros-config" -p standard set --system locale.language en_US.UTF-8 || fail "idioma en la capa del sistema"
"$B/rros-apply" -p standard --root "$T/sysroot" >/dev/null
grep -q '^LANG=en_US.UTF-8' "$T/sysroot/etc/default/locale" || fail "cambiar el idioma"
"$B/rros-config" -p standard unset --system locale.language
"$B/rros-config" -p standard set security.usb_storage false
"$B/rros-config" -p standard set security.firewall_allow '8080/tcp,6000-6010/udp'
"$B/rros-apply" -p standard --root "$T/sysroot" >/dev/null
grep -q 'install usb-storage /bin/false' "$T/sysroot/etc/modprobe.d/rros-security.conf" || fail "bloqueo de USB"
grep -q 'udp dport 6000-6010 accept' "$T/sysroot/etc/nftables.conf" || fail "puertos abiertos"
if "$B/rros-config" -p standard set security.firewall_allow '22; flush ruleset' 2>/dev/null; then fail "puerto con órdenes aceptado"; fi
"$B/rros-config" -p standard unset security.usb_storage
"$B/rros-config" -p standard unset security.firewall_allow

# --- en Arch: locale.conf y locale.gen, faillock sin pam-auth-update, sshd; nada de apt ni /etc/timezone ---
A=$T/archroot
mkdir -p "$A/etc" "$A/usr/lib/systemd/system"
: > "$A/etc/arch-release"
: > "$A/usr/lib/systemd/system/sshd.service"
printf '#en_US.UTF-8 UTF-8\n#es_ES.UTF-8 UTF-8  \n#es_MX.UTF-8 UTF-8\n' > "$A/etc/locale.gen"
ARCH_OUT=$("$B/rros-apply" -p standard --root "$A") || fail "rros-apply en Arch"
grep -q '^LANG=es_ES.UTF-8' "$A/etc/locale.conf" || fail "Arch: locale.conf"
grep -q '^es_ES.UTF-8 UTF-8' "$A/etc/locale.gen" || fail "Arch: locale.gen"
grep -q '^#es_MX' "$A/etc/locale.gen" || fail "Arch: locale.gen solo el idioma elegido"
for f in etc/apt etc/timezone etc/default/locale; do
	[ ! -e "$A/$f" ] || fail "Arch: no debe existir /$f"
done
grep -q 'deny = 10' "$A/etc/security/faillock.conf" || fail "Arch: faillock"
grep -q '^sshd.service' <<<"$ARCH_OUT" || fail "Arch: ssh es sshd.service"
grep -q 'actualizaciones .*se descargarían solas' <<<"$ARCH_OUT" || fail "Arch: actualizaciones"

# --- rros-security: auditoría sobre un sistema falso ---
F=$T/fakeroot
mkdir -p "$F/proc/sys/kernel" "$F/proc/net" "$F/etc" "$F/sys/module/apparmor/parameters" "$T/secbin"
echo 2 > "$F/proc/sys/kernel/kptr_restrict"
echo 0 > "$F/proc/sys/kernel/dmesg_restrict"
echo quiet > "$F/proc/cmdline"
echo Y > "$F/sys/module/apparmor/parameters/enabled"
printf '  sl  local_address rem_address   st\n   0: 00000000:0016 00000000:0000 0A\n   1: 0100007F:0277 00000000:0000 0A\n' > "$F/proc/net/tcp"
printf 'root:x:0:0:root:/root:/bin/bash\ntoor:x:0:0::/:/bin/sh\n' > "$F/etc/passwd"
printf 'table inet rros {\n chain input { policy drop; }\n}\n' > "$F/etc/nftables.conf"
# shellcheck disable=SC2016  # es el contenido literal del systemctl falso
printf '#!/bin/sh\n[ "$3" = nftables.service ] && exit 0\nexit 3\n' > "$T/secbin/systemctl"
chmod +x "$T/secbin/systemctl"
SEC=$(RROS_ROOT=$F PATH="$T/secbin:$PATH" "$B/rros-security" auditoria --json)
python3 - "$SEC" <<'PY' || fail "auditoría de seguridad"
import json, sys
d = json.loads(sys.argv[1])
c = {x["id"]: x for x in d["checks"]}
assert isinstance(d["score"], int) and 0 <= d["score"] <= 100, d["score"]
assert c["firewall"]["status"] == "ok", c["firewall"]
assert "22/tcp" in c["ports"]["detail"] and "631" not in c["ports"]["detail"], c["ports"]
assert c["kernel"]["status"] == "warn" and "1 de 2" in c["kernel"]["detail"], c["kernel"]
assert c["accounts"]["status"] == "fail", c["accounts"]
assert c["apparmor"]["status"] == "ok", c["apparmor"]
assert c["ssh"]["status"] == "ok", c["ssh"]
assert d["level"] == "estandar", d["level"]
PY
RROS_ROOT=$F PATH="$T/secbin:$PATH" "$B/rros-security" | gq 'puntuación' || fail "auditoría en texto"
# AppArmor sin perfiles: aviso en un sistema instalado, solo información en el live
mkdir -p "$F/sys/kernel/security/apparmor"; : > "$F/sys/kernel/security/apparmor/profiles"
aa() { RROS_ROOT=$F PATH="$T/secbin:$PATH" "$B/rros-security" --json | python3 -c 'import json,sys; print([c["status"] for c in json.load(sys.stdin)["checks"] if c["id"]=="apparmor"][0])'; }
[ "$(aa)" = warn ] || fail "AppArmor sin perfiles en un sistema instalado"
echo "boot=casper quiet" > "$F/proc/cmdline"
[ "$(aa)" = info ] || fail "AppArmor sin perfiles en el live"
echo quiet > "$F/proc/cmdline"; rm -r "$F/sys/kernel/security"
# en Arch: faillock en system-auth y actualizaciones con rros-updates.timer
mkdir -p "$F/etc/pam.d" "$F/etc/systemd/system/timers.target.wants"
: > "$F/etc/arch-release"
echo 'auth required pam_faillock.so preauth' > "$F/etc/pam.d/system-auth"
st() { RROS_ROOT=$F PATH="$T/secbin:$PATH" "$B/rros-security" --json | python3 -c 'import json,sys; c={x["id"]:x["status"] for x in json.load(sys.stdin)["checks"]}; print(c["updates"], c["faillock"])'; }
[ "$(st)" = "warn ok" ] || fail "Arch: auditoría sin descarga de actualizaciones ($(st))"
: > "$F/etc/systemd/system/timers.target.wants/rros-updates.timer"
[ "$(st)" = "ok ok" ] || fail "Arch: auditoría de actualizaciones y faillock ($(st))"
rm -r "$F/etc/arch-release" "$F/etc/pam.d" "$F/etc/systemd"
# niveles y puertos se escriben en la capa del sistema
export RROS_SECURITY_NOROOT=1 RROS_SECURITY_NOAPPLY=1
"$B/rros-security" nivel estricto >/dev/null || fail "nivel estricto"
grep -q 'usb_storage = false' "$RROS_SYSTEM_CONF" || fail "el nivel llega a la capa del sistema"
[ "$(RROS_ROOT=$F PATH="$T/secbin:$PATH" "$B/rros-security" --json | python3 -c 'import json,sys; print(json.load(sys.stdin)["level"])')" = estricto ] || fail "nivel detectado"
"$B/rros-security" abrir 8080 >/dev/null
"$B/rros-security" abrir 8080/tcp >/dev/null
"$B/rros-security" abrir 53/udp >/dev/null
grep -q 'firewall_allow = "8080/tcp,53/udp"' "$RROS_SYSTEM_CONF" || fail "abrir puertos sin duplicar"
"$B/rros-security" cerrar 8080 >/dev/null
grep -q 'firewall_allow = "53/udp"' "$RROS_SYSTEM_CONF" || fail "cerrar puerto"
if "$B/rros-security" abrir '1;reboot' 2>/dev/null; then fail "puerto no válido aceptado"; fi
if "$B/rros-security" nivel paranoico 2>/dev/null; then fail "nivel desconocido aceptado"; fi
"$B/rros-security" nivel estandar --dry-run | gq 'simulación' || fail "nivel en simulación"
unset RROS_SECURITY_NOROOT RROS_SECURITY_NOAPPLY
rm -f "$RROS_SYSTEM_CONF"

# --- rros-devices: acciones sobre un /sys falso, con udisksctl y rfkill falsos ---
D=$T/devroot
mkdir -p "$D/sys/block" "$D/sys/devices/usb1/sdb/sdb1" "$D/sys/devices/ata/sda/sda1" "$D/proc/self" "$D/run/udev/data" "$T/devbin"
echo 62521344 > "$D/sys/devices/usb1/sdb/size"; echo 1 > "$D/sys/devices/usb1/sdb/removable"
echo 1 > "$D/sys/devices/usb1/sdb/sdb1/partition"; echo 1000 > "$D/sys/devices/usb1/sdb/sdb1/size"; echo 8:17 > "$D/sys/devices/usb1/sdb/sdb1/dev"
echo 2000000 > "$D/sys/devices/ata/sda/size"; echo 0 > "$D/sys/devices/ata/sda/removable"
echo 1 > "$D/sys/devices/ata/sda/sda1/partition"; echo 1000 > "$D/sys/devices/ata/sda/sda1/size"; echo 8:1 > "$D/sys/devices/ata/sda/sda1/dev"
ln -s ../devices/usb1/sdb "$D/sys/block/sdb"; ln -s ../devices/ata/sda "$D/sys/block/sda"
echo 'E:ID_FS_TYPE=vfat' > "$D/run/udev/data/b8:17"; echo 'E:ID_FS_TYPE=ext4' > "$D/run/udev/data/b8:1"
printf '/dev/sda1 / ext4 rw 0 0\n/dev/sdb1 /media/rros/USB vfat rw 0 0\n' > "$D/proc/self/mounts"
mkdir -p "$D/sys/class/rfkill/rfkill0"; echo wlan > "$D/sys/class/rfkill/rfkill0/type"; echo 0 > "$D/sys/class/rfkill/rfkill0/soft"
for b in udisksctl rfkill eject; do printf '#!/bin/sh\necho "%s $*" >> "%s"\n' "$b" "$T/dev-calls.txt" > "$T/devbin/$b"; chmod +x "$T/devbin/$b"; done
dev() { RROS_ROOT=$D PATH="$T/devbin:$PATH" "$B/rros-devices" "$@"; }
python3 - "$(dev lista --json)" <<'PY' || fail "rros-devices JSON"
import json, sys
d = {x["id"]: x for x in json.loads(sys.argv[1])["devices"]}
assert d["sdb"]["actions"] == "eject" and "USB" in d["sdb"]["detail"], d["sdb"]
assert d["sdb1"]["actions"] == "unmount" and d["sdb1"]["parent"] == "sdb", d["sdb1"]
assert d["sda1"]["actions"] == "" and d["sda1"]["state"].startswith("sistema"), d["sda1"]
assert d["rfkill0"]["actions"] == "block", d["rfkill0"]
PY
dev lista disco | gq 'Discos y memorias' || fail "rros-devices lista"
: > "$T/dev-calls.txt"
dev desmontar sdb1 >/dev/null || fail "desmontar"
if dev desmontar sda1 2>/dev/null; then fail "desmontar el disco del sistema"; fi
if dev montar '../../etc' 2>/dev/null; then fail "identificador peligroso aceptado"; fi
if dev expulsar sda 2>/dev/null; then fail "expulsar un disco interno"; fi
dev expulsar sdb >/dev/null || fail "expulsar"
dev radio wifi off >/dev/null || fail "radio"
if dev radio 'wifi;reboot' off 2>/dev/null; then fail "radio no válida aceptada"; fi
[ "$(cat "$T/dev-calls.txt")" = "udisksctl unmount -b /dev/sdb1
udisksctl unmount -b /dev/sdb1
udisksctl power-off -b /dev/sdb
rfkill block wifi" ] || fail "órdenes de dispositivos: $(cat "$T/dev-calls.txt")"

# --- rros-new: plantillas y proyectos ---
NH=$T/newhome
mkdir -p "$NH/trabajo"
TPL=$PWD/templates
new() { HOME=$NH XDG_CONFIG_HOME=$NH/.config RROS_TEMPLATES_DIR=$TPL "$B/rros-new" "$@"; }
new lista | gq '^  proyecto-python ' || fail "rros-new lista"
for id in $(new lista | awk '/^  [a-z]/ && !/proyecto/ {print $1}'); do
	new "$id" "prueba-$id" --dir "$NH/trabajo" >/dev/null || fail "rros-new $id"
done
if [ ! -x "$NH/trabajo/prueba-bash.sh" ] || [ ! -x "$NH/trabajo/prueba-python.py" ]; then fail "los scripts se crean ejecutables"; fi
[ ! -x "$NH/trabajo/prueba-web.html" ] || fail "una página web no es ejecutable"
grep -q '<title>prueba-web</title>' "$NH/trabajo/prueba-web.html" || fail "marcador {{NOMBRE}}"
grep -q "Fecha: $(date +%Y-%m-%d)" "$NH/trabajo/prueba-nota.md" || fail "marcador {{FECHA}}"
! grep -rq '{{' "$NH/trabajo" || fail "quedan marcadores sin sustituir"
bash -n "$NH/trabajo/prueba-bash.sh" || fail "el script de Bash es válido"
[ -f "$NH/.local/share/applications/prueba-lanzador.desktop" ] || [ -f "$NH/trabajo/prueba-lanzador.desktop" ] || fail "lanzador"
if new texto prueba-texto --dir "$NH/trabajo" 2>/dev/null; then fail "rros-new pisa un archivo existente"; fi
echo cambiado > "$NH/trabajo/prueba-texto.txt"
new texto prueba-texto --dir "$NH/trabajo" --force >/dev/null || fail "rros-new --force"
[ ! -s "$NH/trabajo/prueba-texto.txt" ] || fail "--force reemplaza el contenido"
(cd "$NH/trabajo" && new proyecto-python mi-app >/dev/null) || fail "proyecto python"
if [ ! -x "$NH/trabajo/mi-app/main.py" ] || [ ! -f "$NH/trabajo/mi-app/.gitignore" ]; then fail "proyecto completo"; fi
grep -q '^# mi-app' "$NH/trabajo/mi-app/README.md" || fail "marcadores en proyectos"
if (cd "$NH/trabajo" && new proyecto-python mi-app 2>/dev/null); then fail "un proyecto existente se pisa"; fi
if command -v cc >/dev/null 2>&1 && command -v make >/dev/null 2>&1; then
	(cd "$NH/trabajo" && new proyecto-c hola >/dev/null && cd hola && make -s >/dev/null && [ "$(./hola)" = "Hola desde hola" ]) || fail "el proyecto en C compila"
fi
new exportar "$NH/Plantillas" | gq 'copiadas' || fail "rros-new exportar"
[ -x "$NH/Plantillas/Script de Bash.sh" ] || fail "plantillas exportadas"
! grep -q '{{' "$NH/Plantillas/Nota.md" || fail "plantillas exportadas sin marcadores"
printf 'Estimado {{USUARIO}}\n' > "$NH/Plantillas/Carta.txt"
new lista | gq 'Carta.txt' || fail "plantillas del usuario en la lista"
new Carta para-ana --dir "$NH/trabajo" >/dev/null || fail "plantilla del usuario"
grep -q "Estimado $(id -un)" "$NH/trabajo/para-ana.txt" || fail "marcadores en plantillas del usuario"
if new texto -x 2>/dev/null; then fail "nombre que empieza por guion aceptado"; fi
if new noexiste algo 2>/dev/null; then fail "plantilla inexistente aceptada"; fi
# --- temas, atajos, reglas, instantáneas (consola) ---
export RROS_THEMES_DIR=$T/themes
"$B/rros-theme" -p standard list | gq '^\* lirio' || fail "tema por defecto"
"$B/rros-theme" -p standard use nord >/dev/null
"$B/rros-theme" -p standard list | gq '^\* nord' || fail "rros-theme use"
grep -q 'Color=46,52,64' "$XDG_DATA_HOME/konsole/Lily.colorscheme" || fail "el tema llega a la terminal"
if "$B/rros-theme" use no-existe 2>/dev/null; then fail "tema inexistente debía fallar"; fi
"$B/rros-theme" -p standard cycle >/dev/null
"$B/rros-theme" -p standard list | gq '^\* dracula' || fail "rros-theme cycle"
"$B/rros-theme" new miotema --base nord | gq 'creado' || fail "rros-theme new"
if "$B/rros-theme" new miotema 2>/dev/null; then fail "new debía negarse a pisar un tema"; fi
if "$B/rros-theme" new nord 2>/dev/null; then fail "new debía negarse con nombre incluido"; fi
sed -i 's/^bg = "#2e3440"/bg = "#101820"/' "$RROS_THEMES_DIR/miotema.toml"
"$B/rros-theme" -p standard use miotema >/dev/null
grep -q 'Color=16,24,32' "$XDG_DATA_HOME/konsole/Lily.colorscheme" || fail "tema propio aplicado"
"$B/rros-theme" -p standard list | gq 'miotema *propio' || fail "tema propio en la lista"

"$B/rros-config" set keybind.nav 'Mod+Shift+b firefox'
"$B/rros-config" set rule.ff 'app=firefox maximized'
"$B/rros-config" set autostart.sync 'syncthing -no-browser'
"$B/rros-theme" -p standard apply --quiet
grep -q '_launch=Meta+Shift+B' "$XDG_CONFIG_HOME/kglobalshortcutsrc" || fail "atajo en KDE"
grep -q 'Exec=firefox' "$XDG_DATA_HOME/applications/rros-kb-nav.desktop" || fail "lanzador del atajo"
grep -q 'wmclass=firefox' "$XDG_CONFIG_HOME/kwinrulesrc" || fail "regla en KWin"
grep -q 'Exec=syncthing -no-browser' "$XDG_CONFIG_HOME/autostart/rros-auto-sync.desktop" || fail "programa de inicio"
for bad in "keybind.x Mod+b" "rule.x app=a vuela" "keybind.MAL Mod+b x" "autostart.x"; do
	# shellcheck disable=SC2086
	if "$B/rros-config" set $bad 2>/dev/null; then fail "debía fallar: $bad"; fi
done
"$B/rros-config" validate

# exportar, instantáneas, importar, reset
"$B/rros-config" export > "$T/export.toml"
grep -q 'ui.theme\|theme = "miotema"' "$T/export.toml" || fail "export"
"$B/rros-config" snapshot save base
"$B/rros-config" set ui.dock_icon_size 64
"$B/rros-config" snapshot list | gq -x base || fail "snapshot list"
"$B/rros-config" snapshot restore base
eq "$("$B/rros-config" -p standard get ui.dock_icon_size)" 48  # el valor que había al guardar la instantánea
if "$B/rros-config" snapshot restore ../x 2>/dev/null; then fail "nombre de snapshot inválido"; fi
"$B/rros-config" import "$T/export.toml"
if "$B/rros-config" reset 2>/dev/null; then fail "reset sin --yes debía fallar"; fi
"$B/rros-config" reset --yes
eq "$("$B/rros-config" -p standard get ui.theme)" lirio
printf '[ui]\ndock_icon_size = 9999\n' > "$T/mal-import.toml"
if "$B/rros-config" import "$T/mal-import.toml" 2>/dev/null; then fail "import inválido debía fallar"; fi
"$B/rros-config" help | gq 'keybind' || fail "ayuda con ejemplos"
# --- comando rros: subcomandos y extensiones ---
"$B/rros" settings help | gq 'tui' || fail "rros settings"
"$B/rros" config help | gq 'keybind' || fail "rros config help"
mkdir -p "$T/bin"
printf '#!/bin/sh\necho "extension $*"\n' > "$T/bin/rros-saluda"
chmod +x "$T/bin/rros-saluda"
eq "$(PATH="$T/bin:$PATH" "$B/rros" saluda hola mundo)" "extension hola mundo"
if PATH="$T/bin:$PATH" "$B/rros" '../saluda' 2>/dev/null; then fail "rros no debe aceptar rutas como extensión"; fi
if "$B/rros" noexiste 2>/dev/null; then fail "rros noexiste debía fallar"; fi

# --- tienda de aplicaciones: rros-apps (dpkg-query, pacman y flatpak simulados) ---
mkdir -p "$T/fakebin"
# shellcheck disable=SC2016
printf '#!/bin/sh\nprintf "installed mpv\\nnot-installed vlc\\n"\n' > "$T/fakebin/dpkg-query"
# shellcheck disable=SC2016
printf '#!/bin/sh\n[ "$1" = "list" ] && { echo org.signal.Signal; exit 0; }\nexit 1\n' > "$T/fakebin/flatpak"
# shellcheck disable=SC2016
printf '#!/bin/sh\n[ "$1" = "-Qq" ] && { echo mpv; exit 0; }\nexit 1\n' > "$T/fakebin/pacman"
chmod +x "$T/fakebin/dpkg-query" "$T/fakebin/flatpak" "$T/fakebin/pacman"
export RROS_APPS_CATALOG=$PWD/apps/catalog.toml
APPS="env PATH=$T/fakebin:$PATH RROS_PKG=apt $B/rros-apps"
PAPPS="env PATH=$T/fakebin:$PATH RROS_PKG=pacman $B/rros-apps"
# la salida se captura antes de buscar: con pipefail, `grep -q` cortaría la tubería (SIGPIPE) y daría falso error
LIST=$($APPS list)
grep -q 'mpv .*instalada' <<<"$LIST" || fail "apps: mpv debe figurar instalada"
grep -q 'signal .*instalada' <<<"$LIST" || fail "apps: Signal (Flatpak) debe figurar instalada"
grep -q 'brave .* -$' <<<"$LIST" || fail "apps: Brave sin instalar"
grep -q 'firefox .*incluida' <<<"$LIST" || fail "apps: Firefox incluido en el perfil"
[ "$(wc -l <<<"$LIST")" -ge 50 ] || fail "apps: catálogo con menos de 50 entradas"
GAMES=$($APPS list --category games)
grep -q steam <<<"$GAMES" || fail "apps: filtro por categoría"
if grep -q firefox <<<"$GAMES"; then fail "apps: el filtro no excluye otras categorías"; fi
grep -q firefox <<<"$($APPS search navegador)" || fail "apps: búsqueda"
grep -q 'org.mozilla.firefox' <<<"$($APPS info firefox)" || fail "apps: info"
FF=$($APPS install firefox --dry-run)
grep -Eq 'flatpak install --(user|system) -y --noninteractive flathub org.mozilla.firefox' <<<"$FF" || fail "apps: instalar por Flatpak"
grep -Eq 'flatpak remote-add --(user|system) --if-not-exists flathub' <<<"$FF" || fail "apps: añade Flathub"
GIMP=$($APPS install gimp --dry-run)
grep -q 'apt-get update' <<<"$GIMP" || fail "apps: apt-get update antes de instalar"
grep -q 'apt-get install -y gimp' <<<"$GIMP" || fail "apps: instalar por apt"
grep -q 'com.obsproject.Studio' <<<"$($APPS install obs --flatpak --dry-run)" || fail "apps: --flatpak elige Flatpak"
grep -q 'apt-get remove -y vlc' <<<"$($APPS remove vlc --dry-run)" || fail "apps: quitar"
grep -q 'ya está instalada' <<<"$($APPS install mpv)" || fail "apps: no reinstala lo instalado"
# en Arch: pacman (siempre -Syu, nunca una actualización a medias), con los paquetes de idioma
grep -q 'mpv .*instalada' <<<"$($PAPPS list)" || fail "apps pacman: mpv debe figurar instalada"
grep -q 'pacman -Syu --needed --noconfirm firefox firefox-i18n-es-es$' <<<"$($PAPPS install firefox --dry-run)" || fail "apps pacman: instalar"
grep -q 'pacman -Rns --noconfirm vlc$' <<<"$($PAPPS remove vlc --dry-run)" || fail "apps pacman: quitar"
grep -q 'pacman -Syu --noconfirm$' <<<"$($PAPPS update --dry-run)" || fail "apps pacman: actualizar"
grep -q 'com.valvesoftware.Steam' <<<"$($PAPPS install steam --dry-run)" || fail "apps pacman: sin paquete, Flatpak"
grep -q 'pacman: *libreoffice-fresh,libreoffice-fresh-es' <<<"$($PAPPS info libreoffice)" || fail "apps pacman: info"
grep -q 'ya está instalada' <<<"$($PAPPS install mpv)" || fail "apps pacman: no reinstala lo instalado"
for bad in "nada" "x;y" '../etc' 'FIREFOX'; do
	if $APPS install "$bad" --dry-run 2>/dev/null; then fail "apps: id no válido aceptado: $bad"; fi
done
JSON=$($APPS list --json)
python3 -c 'import json,sys; d=json.load(sys.stdin); assert len(d)>=50 and all("id" in x and "installed" in x for x in d)' <<<"$JSON" || fail "apps: JSON"
# el catálogo es coherente: categorías conocidas y todo tiene un origen de instalación
python3 -c '
import json,sys
cats={"internet","office","media","graphics","dev","system","games","education","utility"}
for a in json.load(sys.stdin):
    assert a["category"] in cats, a["id"]
    assert a["flatpak"] or a["pacman"], a["id"]
' <<<"$JSON" || fail "apps: catálogo incoherente"

# --- instalador: la parte que no toca discos (plan y validación de argumentos) ---
INST=$PWD/overlay/common/usr/bin/rros-install
inst_out=$(PATH=$B:$PATH "$INST" plan --disk-gb 14 --bios -p lite)
case $inst_out in *rros-root*ext4*) ;; *) fail "rros-install plan" ;; esac
if PATH=$B:$PATH "$INST" --user 'Mal Usuario' --disk /dev/null --dry-run 2>/dev/null; then fail "rros-install acepta un usuario no válido"; fi
if PATH=$B:$PATH "$INST" --hostname 'a;b' --disk /dev/null --dry-run 2>/dev/null; then fail "rros-install acepta un nombre de equipo no válido"; fi
# shellcheck disable=SC2016  # el $( ) literal es justo lo que se prueba
if PATH=$B:$PATH "$INST" --cmdline 'x$(id)' --disk /dev/null --dry-run 2>/dev/null; then fail "rros-install acepta un --cmdline peligroso"; fi
if PATH=$B:$PATH "$INST" --disk /dev/null --dry-run 2>/dev/null; then fail "rros-install acepta un dispositivo que no es un disco"; fi
if PATH=$B:$PATH "$INST" --locale 'es;ES' --disk /dev/null --dry-run 2>/dev/null; then fail "rros-install acepta un idioma no válido"; fi
if PATH=$B:$PATH "$INST" --fullname 'Ana:0:0' --disk /dev/null --dry-run 2>/dev/null; then fail "rros-install acepta un nombre completo con ':'"; fi

# discos para el instalador sobre un /sys falso: NVMe con Windows (ESP, MSR, NTFS) y 40 GiB libres
DR=$T/disks
mkpart() {   # disco parte inicio_mib tamaño_mib devnum udev...
	local d=$DR/sys/devices/pci/$1/$2
	mkdir -p "$d" "$DR/run/udev/data"
	echo 1 > "$d/partition"; echo $(($3 * 2048)) > "$d/start"; echo $(($4 * 2048)) > "$d/size"; echo "$5" > "$d/dev"
	shift 5
	printf 'E:%s\n' "$@" > "$DR/run/udev/data/b$(cat "$d/dev")"
}
mkdir -p "$DR/sys/devices/pci/nvme0n1/device" "$DR/sys/block" "$DR/run/udev/data" "$DR/proc/self"
echo 209715200 > "$DR/sys/devices/pci/nvme0n1/size"   # 100 GiB
echo 259:0 > "$DR/sys/devices/pci/nvme0n1/dev"
echo "Disco de prueba" > "$DR/sys/devices/pci/nvme0n1/device/model"
echo E:ID_PART_TABLE_TYPE=gpt > "$DR/run/udev/data/b259:0"
mkpart nvme0n1 nvme0n1p1 1 100 259:1 ID_FS_TYPE=vfat ID_PART_ENTRY_TYPE=c12a7328-f81f-11d2-ba4b-00a0c93ec93b
mkpart nvme0n1 nvme0n1p2 101 16 259:2 ID_PART_ENTRY_TYPE=e3c9e316-0b5c-4db8-817d-f92df00215ae
mkpart nvme0n1 nvme0n1p3 117 61440 259:3 ID_FS_TYPE=ntfs ID_FS_LABEL=Windows
ln -s ../devices/pci/nvme0n1 "$DR/sys/block/nvme0n1"
: > "$DR/proc/self/mounts"
dj=$(RROS_ROOT=$DR "$B/rros-installer" disks --json --uefi -p lite)
python3 -c '
import json,sys
j=json.load(sys.stdin)
d=j["disks"][0]
assert j["firmware"]=="uefi" and j["min_mb"]==12288, j
assert d["path"]=="/dev/nvme0n1" and d["systems"]=="Windows" and d["windows"], d
assert d["esp"]=="nvme0n1p1" and d["free_mb"]==102400-1-117-61440, d
assert d["wipe"]=="" and d["alongside"]=="" and d["alongside_crypt"]=="", d
assert [p["role"] for p in d["parts"]]==["esp","windows","windows"], d["parts"]
' <<<"$dj" || fail "rros-installer disks --json"
RROS_ROOT=$DR "$B/rros-installer" disks --uefi -p lite | gq 'nvme0n1.*gpt.*Windows' || fail "rros-installer disks (texto)"
tbl=$(RROS_ROOT=$DR "$B/rros-installer" plan --disk /dev/nvme0n1 --alongside --encrypt --uefi -p lite --table)
[ "$tbl" = "esp 1 100 vfat /boot/efi 1 0
boot 61557 1024 ext4 /boot 0 0
root 62581 39818 ext4 / 0 1" ] || fail "plan junto a Windows: $tbl"
RROS_ROOT=$DR "$B/rros-installer" plan --disk /dev/nvme0n1 --alongside --uefi -p lite --sfdisk | gq -x 'start=61557MiB, size=40842MiB, type=L, name=rros-root' ||
	fail "plan junto a Windows (sfdisk)"
# el live (montado) no se ofrece
echo "/dev/nvme0n1p3 /cdrom ntfs ro 0 0" > "$DR/proc/self/mounts"
if RROS_ROOT=$DR "$B/rros-installer" plan --disk nvme0n1 --uefi -p lite >/dev/null 2>&1; then fail "plan acepta el disco del live"; fi

# --- rros-settings api: lo que usa la aplicación Lily Ajustes (sin servidor ni navegador) ---
export RROS_NO_APPLY=1
st=$("$B/rros-settings" api GET /api/state) || fail "api state"
python3 -c 'import json,sys; d=json.load(sys.stdin); assert d["keys"] and d["desktops"] and d["themes"], d.keys()' <<<"$st" || fail "api state: JSON"
"$B/rros-settings" api POST /api/set 'key=ui.dock_style&value=3d' | gq '"ok":true' || fail "api set"
eq "$("$B/rros-config" get ui.dock_style)" 3d
if "$B/rros-settings" api POST /api/set 'key=ui.dock_style&value=4d' >/dev/null; then fail "api set debía rechazar 4d"; fi
bad_out=$("$B/rros-settings" api POST /api/set 'key=ui.dock_style&value=4d' || true)
case $bad_out in *'"error"'*) ;; *) fail "api set: mensaje de error" ;; esac
"$B/rros-settings" api POST /api/unset 'key=ui.dock_style' | gq '"ok":true' || fail "api unset"
"$B/rros-settings" api GET /api/apps | gq '"id":"firefox"' || fail "api apps"
case $("$B/rros-settings" api GET /api/nada || true) in *'ruta desconocida'*) ;; *) fail "api ruta desconocida" ;; esac
if "$B/rros-settings" api GET /api/state x y 2>/dev/null; then fail "api con argumentos de más"; fi
unset RROS_NO_APPLY

# --- rros-max-hz: cada pantalla a su frecuencia más alta con la misma resolución (JSON con el formato real de
#     kscreen-doctor -j; la que ya está al máximo o tiene refreshRate null no se toca) ---
if command -v jq >/dev/null 2>&1; then
	mkdir -p "$T/hzbin"
	cat > "$T/hz.json" <<'JSON'
{"outputs":[
 {"name":"DP-1","enabled":true,"connected":true,"currentModeId":"1","modes":[
  {"id":"1","refreshRate":60.0,"size":{"width":2560,"height":1440}},
  {"id":"2","refreshRate":119.98,"size":{"width":2560,"height":1440}},
  {"id":"3","refreshRate":165.0,"size":{"width":1920,"height":1080}}]},
 {"name":"Virtual-1","enabled":true,"connected":true,"currentModeId":"7","modes":[
  {"id":"7","refreshRate":null,"size":{"width":1280,"height":800}}]},
 {"name":"HDMI-1","enabled":false,"connected":true,"currentModeId":"4","modes":[
  {"id":"4","refreshRate":30.0,"size":{"width":1920,"height":1080}},{"id":"5","refreshRate":60.0,"size":{"width":1920,"height":1080}}]}]}
JSON
	printf '#!/bin/sh
[ "$1" = -j ] && { cat "%s"; exit 0; }
echo "$*" >> "%s"
' "$T/hz.json" "$T/hz-calls.txt" > "$T/hzbin/kscreen-doctor"
	printf '#!/bin/sh
[ "$2" = display.max_refresh ] && echo "${HZ_ON:-true}"
' > "$T/hzbin/rros-config"
	chmod +x "$T/hzbin/kscreen-doctor" "$T/hzbin/rros-config"
	PATH=$T/hzbin:$PATH overlay/common/usr/libexec/rros-max-hz >/dev/null
	eq "$(cat "$T/hz-calls.txt")" "output.DP-1.mode.2"
	rm -f "$T/hz-calls.txt"
	HZ_ON=false PATH=$T/hzbin:$PATH overlay/common/usr/libexec/rros-max-hz >/dev/null
	[ ! -e "$T/hz-calls.txt" ] || fail "rros-max-hz con display.max_refresh = false"
fi

# --- interfaces: rros-settings (TUI en un pty) ---
if command -v python3 >/dev/null 2>&1; then
	python3 tests/tui_test.py || fail "TUI"
fi
echo "  cli OK"
