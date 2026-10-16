#!/usr/bin/env bash
# Arranca rros-settings contra una configuración temporal y ejecuta las pruebas de la GUI con un navegador.
# Necesita Playwright: PWPATH=<ruta a playwright> tests/gui_run.sh
set -euo pipefail
cd "$(dirname "$0")/.."
: "${PWPATH:?define PWPATH con la ruta a playwright}"
T=$(mktemp -d)
trap 'curl -s -H "X-RROS-Token: tk" -X POST "http://127.0.0.1:18769/api/quit" >/dev/null 2>&1 || true; rm -rf "${T:?}"' EXIT
mkdir -p "$T/bin"
# terminal falsa: anota con qué argumentos la abre la tienda
printf '#!/bin/sh\nprintf "%%s\\n" "$@" > "%s/foot-args.txt"\n' "$T" > "$T/bin/foot"
chmod +x "$T/bin/foot"
# dispositivos y seguridad sobre un sistema falso (RROS_ROOT), con udisksctl, rfkill y systemctl falsos
R=$T/root
mkdir -p "$R/sys/block" "$R/sys/devices/usb1/sdb/sdb1" "$R/proc/self" "$R/proc/sys/kernel" "$R/run/udev/data" \
	"$R/sys/class/rfkill/rfkill0" "$R/etc" "$T/home/Documentos"
echo 62521344 > "$R/sys/devices/usb1/sdb/size"; echo 1 > "$R/sys/devices/usb1/sdb/removable"
echo Kingston > "$R/sys/devices/usb1/sdb/vendor"
echo 1 > "$R/sys/devices/usb1/sdb/sdb1/partition"; echo 1000 > "$R/sys/devices/usb1/sdb/sdb1/size"; echo 8:17 > "$R/sys/devices/usb1/sdb/sdb1/dev"
ln -s ../devices/usb1/sdb "$R/sys/block/sdb"
echo 'E:ID_FS_TYPE=vfat' > "$R/run/udev/data/b8:17"
echo '/dev/sdb1 /media/rros/USB vfat rw 0 0' > "$R/proc/self/mounts"
echo wlan > "$R/sys/class/rfkill/rfkill0/type"; echo 0 > "$R/sys/class/rfkill/rfkill0/soft"
echo quiet > "$R/proc/cmdline"; echo 2 > "$R/proc/sys/kernel/kptr_restrict"
printf 'table inet rros {\n chain input { policy drop; }\n}\n' > "$R/etc/nftables.conf"
for b in udisksctl rfkill; do printf '#!/bin/sh\necho "%s $*" >> "%s/dev-calls.txt"\n' "$b" "$T" > "$T/bin/$b"; done
# shellcheck disable=SC2016  # contenido literal del systemctl falso
printf '#!/bin/sh\n[ "$3" = nftables.service ]\n' > "$T/bin/systemctl"
chmod +x "$T/bin/udisksctl" "$T/bin/rfkill" "$T/bin/systemctl"
export RROS_PROFILE_DIR=$PWD/profiles RROS_PROFILE_FILE=$T/profile RROS_USER_CONF=$T/u.toml \
	RROS_SYSTEM_CONF=$T/s.toml XDG_CONFIG_HOME=$T/xdg RROS_THEMES_DIR=$T/themes RROS_GUI_DIR=$PWD/gui \
	RROS_APPS_CATALOG=$PWD/apps/catalog.toml PATH="$T/bin:$PATH" RROS_ROOT=$R \
	RROS_TEMPLATES_DIR=$PWD/templates HOME=$T/home
build/rros-settings web --no-open --port 18769 --token tk --timeout 300 > "$T/server.log" 2>&1 &
sleep 0.8
RROS_TEST_DIR=$T PORT=18769 TOKEN=tk node tests/gui_test.js
PWPATH=$PWPATH node tests/gui_demo_test.js
PWPATH=$PWPATH node tests/preview_test.js
