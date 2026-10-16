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
"$B/rros-config" -p lite list | grep -q 'ui.effects = "advanced"  (usuario)' || fail "list/capa usuario"
"$B/rros-config" -p lite list | grep -q 'ui.blur = false  (perfil)' || fail "list/capa perfil"
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
"$B/rros-budget" disk "$T/rootfs" -p lite | grep -q 'status=ok' || fail "budget disk ok"
mkdir -p "$T/fakeroot/proc" && printf 'MemTotal: 4194304 kB\nMemAvailable: 1048576 kB\n' > "$T/fakeroot/proc/meminfo"
printf 'processor : 0\nprocessor : 1\nprocessor : 2\nprocessor : 3\n' > "$T/fakeroot/proc/cpuinfo"
RROS_ROOT=$T/fakeroot "$B/rros-budget" ram -p pro | grep -q 'status=ok' || fail "budget ram pro"
if RROS_ROOT=$T/fakeroot "$B/rros-budget" ram -p lite >/dev/null; then fail "budget ram lite debía superar"; fi

# hwdetect con raíz falsa y salidas
eq "$("$B/rros-hwdetect" --root "$T/fakeroot" --profile)" standard
"$B/rros-hwdetect" --json | grep -q '"profile"' || fail "json"
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
"$B/rros-fetch" --plain | grep -q 'Perfil' || fail "rros-fetch"
"$B/rros-fetch" --plain | grep -q 'SO\|Sistema' || fail "rros-fetch sistema"
"$B/rros" version | grep -q 'Aurora' || fail "rros version"
"$B/rros" doctor | grep -q 'sin snap' || fail "rros doctor"
"$B/rros" info --plain | grep -q 'Perfil' || fail "rros info"
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
echo "  cli OK"
