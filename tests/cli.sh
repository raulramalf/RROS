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
eq "$("$B/rros-config" -f profiles/lite.toml get build.libc)" musl

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
"$B/rros-theme" -p lite apply --quiet
for f in sway/config sway/rros-effects.conf waybar/config waybar/style.css foot/foot.ini; do
	[ -s "$XDG_CONFIG_HOME/$f" ] || fail "rros-theme no generó $f"
done
grep -q 'gaps inner 4' "$XDG_CONFIG_HOME/sway/config" || fail "gap de Lite"
sed 1d "$XDG_CONFIG_HOME/waybar/config" | python3 -m json.tool >/dev/null || fail "waybar JSON inválido"
"$B/rros-config" -p lite set ui.gap 20
"$B/rros-theme" -p lite apply --quiet
grep -q 'gaps inner 20' "$XDG_CONFIG_HOME/sway/config" || fail "el ajuste no llega al escritorio"
# alternar oscuro/claro
"$B/rros-theme" -p lite toggle >/dev/null
eq "$("$B/rros-config" -p lite get ui.color_scheme)" light
grep -q 'background=f5f7ff' "$XDG_CONFIG_HOME/foot/foot.ini" || fail "tema claro"
"$B/rros-theme" -p lite toggle >/dev/null
eq "$("$B/rros-config" -p lite get ui.color_scheme)" dark
# no pisa archivos manuales: hace copia
echo "# mio" > "$XDG_CONFIG_HOME/foot/foot.ini"
"$B/rros-theme" -p lite apply --quiet 2>/dev/null
[ "$(cat "$XDG_CONFIG_HOME/foot/foot.ini.bak-rros")" = "# mio" ] || fail "copia del archivo manual"
# ficha y diagnóstico
"$B/rros-fetch" --plain | gq 'Perfil' || fail "rros-fetch"
"$B/rros-fetch" --plain | gq 'SO\|Sistema' || fail "rros-fetch sistema"
"$B/rros" version | gq 'Aurora' || fail "rros version"
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
# --- temas, atajos, reglas, instantáneas (consola) ---
export RROS_THEMES_DIR=$T/themes
"$B/rros-theme" -p standard list | gq '^\* aurora' || fail "tema por defecto"
"$B/rros-theme" -p standard use nord >/dev/null
"$B/rros-theme" -p standard list | gq '^\* nord' || fail "rros-theme use"
grep -q 'background=2e3440' "$XDG_CONFIG_HOME/foot/foot.ini" || fail "el tema llega a foot"
grep -q 'window_bg_color #2e3440' "$XDG_CONFIG_HOME/gtk-4.0/gtk.css" || fail "el tema llega a GTK"
grep -q 'background-color=#2e3440' "$XDG_CONFIG_HOME/mako/config" || fail "el tema llega a mako"
if "$B/rros-theme" use no-existe 2>/dev/null; then fail "tema inexistente debía fallar"; fi
"$B/rros-theme" -p standard cycle >/dev/null
"$B/rros-theme" -p standard list | gq '^\* dracula' || fail "rros-theme cycle"
"$B/rros-theme" new miotema --base nord | gq 'creado' || fail "rros-theme new"
if "$B/rros-theme" new miotema 2>/dev/null; then fail "new debía negarse a pisar un tema"; fi
if "$B/rros-theme" new nord 2>/dev/null; then fail "new debía negarse con nombre incluido"; fi
sed -i 's/^bg = "#2e3440"/bg = "#101820"/' "$RROS_THEMES_DIR/miotema.toml"
"$B/rros-theme" -p standard use miotema >/dev/null
grep -q 'background=101820' "$XDG_CONFIG_HOME/foot/foot.ini" || fail "tema propio aplicado"
"$B/rros-theme" -p standard list | gq 'miotema *propio' || fail "tema propio en la lista"

"$B/rros-config" set keybind.nav 'Mod+Shift+b exec firefox'
"$B/rros-config" set rule.ff 'app_id=firefox workspace 2'
"$B/rros-config" set autostart.sync 'syncthing -no-browser'
"$B/rros-theme" -p standard apply --quiet
# shellcheck disable=SC2016
grep -q 'bindsym $mod+Shift+b exec firefox' "$XDG_CONFIG_HOME/sway/config" || fail "atajo en sway"
grep -q 'assign \[app_id="firefox"\] workspace number 2' "$XDG_CONFIG_HOME/sway/config" || fail "regla en sway"
grep -q 'exec syncthing -no-browser' "$XDG_CONFIG_HOME/sway/config" || fail "autostart en sway"
for bad in "keybind.x Mod+b rm" "rule.x app_id=a vuela" "keybind.MAL Mod+b exec x" "autostart.x"; do
	# shellcheck disable=SC2086
	if "$B/rros-config" set $bad 2>/dev/null; then fail "debía fallar: $bad"; fi
done
"$B/rros-config" validate

# exportar, instantáneas, importar, reset
"$B/rros-config" export > "$T/export.toml"
grep -q 'ui.theme\|theme = "miotema"' "$T/export.toml" || fail "export"
"$B/rros-config" snapshot save base
"$B/rros-config" set ui.gap 30
"$B/rros-config" snapshot list | gq -x base || fail "snapshot list"
"$B/rros-config" snapshot restore base
eq "$("$B/rros-config" -p standard get ui.gap)" 20  # el valor que había al guardar la instantánea
if "$B/rros-config" snapshot restore ../x 2>/dev/null; then fail "nombre de snapshot inválido"; fi
"$B/rros-config" import "$T/export.toml"
if "$B/rros-config" reset 2>/dev/null; then fail "reset sin --yes debía fallar"; fi
"$B/rros-config" reset --yes
eq "$("$B/rros-config" -p standard get ui.theme)" aurora
printf '[ui]\ngap = 9999\n' > "$T/mal-import.toml"
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

# --- tienda de aplicaciones: rros-apps (dpkg-query y flatpak simulados) ---
mkdir -p "$T/fakebin"
# shellcheck disable=SC2016
printf '#!/bin/sh\n[ "$3" = "mpv" ] && { printf installed; exit 0; }\nexit 1\n' > "$T/fakebin/dpkg-query"
# shellcheck disable=SC2016
printf '#!/bin/sh\n[ "$2" = "org.signal.Signal" ] && exit 0\nexit 1\n' > "$T/fakebin/flatpak"
chmod +x "$T/fakebin/dpkg-query" "$T/fakebin/flatpak"
export RROS_APPS_CATALOG=$PWD/apps/catalog.toml
APPS="env PATH=$T/fakebin:$PATH $B/rros-apps"
# la salida se captura antes de buscar: con pipefail, `grep -q` cortaría la tubería (SIGPIPE) y daría falso error
LIST=$($APPS list)
grep -q 'mpv .*instalada' <<<"$LIST" || fail "apps: mpv debe figurar instalada"
grep -q 'signal .*instalada' <<<"$LIST" || fail "apps: Signal (Flatpak) debe figurar instalada"
grep -q 'firefox .* -$' <<<"$LIST" || fail "apps: Firefox sin instalar"
[ "$(wc -l <<<"$LIST")" -ge 50 ] || fail "apps: catálogo con menos de 50 entradas"
GAMES=$($APPS list --category games)
grep -q steam <<<"$GAMES" || fail "apps: filtro por categoría"
if grep -q firefox <<<"$GAMES"; then fail "apps: el filtro no excluye otras categorías"; fi
grep -q firefox <<<"$($APPS search navegador)" || fail "apps: búsqueda"
grep -q 'org.mozilla.firefox' <<<"$($APPS info firefox)" || fail "apps: info"
FF=$($APPS install firefox --dry-run)
grep -q 'flatpak install --user -y --noninteractive flathub org.mozilla.firefox' <<<"$FF" || fail "apps: instalar por Flatpak"
grep -q 'flatpak remote-add --user --if-not-exists flathub' <<<"$FF" || fail "apps: añade Flathub"
grep -q 'apt-get install -y gimp' <<<"$($APPS install gimp --dry-run)" || fail "apps: instalar por apt"
grep -q 'com.obsproject.Studio' <<<"$($APPS install obs --flatpak --dry-run)" || fail "apps: --flatpak elige Flatpak"
grep -q 'apt-get remove -y vlc' <<<"$($APPS remove vlc --dry-run)" || fail "apps: quitar"
grep -q 'ya está instalada' <<<"$($APPS install mpv)" || fail "apps: no reinstala lo instalado"
for bad in "nada" "x;y" '../etc' 'FIREFOX'; do
	if $APPS install "$bad" --dry-run 2>/dev/null; then fail "apps: id no válido aceptado: $bad"; fi
done
JSON=$($APPS list --json)
python3 -c 'import json,sys; d=json.load(sys.stdin); assert len(d)>=50 and all("id" in x and "installed" in x for x in d)' <<<"$JSON" || fail "apps: JSON"
# el catálogo es coherente: categorías conocidas y todo tiene un origen de instalación
python3 -c '
import json,sys
cats={"internet","office","media","graphics","dev","system","games","utility"}
for a in json.load(sys.stdin):
    assert a["category"] in cats, a["id"]
    assert a["apt"] or a["flatpak"], a["id"]
' <<<"$JSON" || fail "apps: catálogo incoherente"

# --- instalador: la parte que no toca discos (plan y validación de argumentos) ---
INST=$PWD/overlay/common/usr/bin/rros-install
inst_out=$(PATH=$B:$PATH "$INST" plan --disk-gb 8 --bios -p lite)
case $inst_out in *rros-root*ext4*) ;; *) fail "rros-install plan" ;; esac
if PATH=$B:$PATH "$INST" --user 'Mal Usuario' --disk /dev/null --dry-run 2>/dev/null; then fail "rros-install acepta un usuario no válido"; fi
if PATH=$B:$PATH "$INST" --hostname 'a;b' --disk /dev/null --dry-run 2>/dev/null; then fail "rros-install acepta un nombre de equipo no válido"; fi
# shellcheck disable=SC2016  # el $( ) literal es justo lo que se prueba
if PATH=$B:$PATH "$INST" --cmdline 'x$(id)' --disk /dev/null --dry-run 2>/dev/null; then fail "rros-install acepta un --cmdline peligroso"; fi
if PATH=$B:$PATH "$INST" --disk /dev/null --dry-run 2>/dev/null; then fail "rros-install acepta un dispositivo que no es un disco"; fi

# --- interfaces: rros-settings (TUI en un pty y servidor web local) ---
if command -v python3 >/dev/null 2>&1; then
	python3 tests/tui_test.py || fail "TUI"
fi
echo "  cli OK"
