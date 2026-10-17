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
# terminales falsas (la del perfil es konsole; foot en los demás escritorios): guardan sus argumentos
for term in foot konsole; do
	printf '#!/bin/sh\nprintf "%%s\\n" "$@" > "%s/foot-args.txt"\n' "$T" > "$T/bin/$term"
	chmod +x "$T/bin/$term"
done
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
printf '#!/bin/sh\necho "$*" >> "%s/a11y.txt"\n' "$T" > "$T/bin/rros-a11y"
chmod +x "$T/bin/udisksctl" "$T/bin/rfkill" "$T/bin/systemctl" "$T/bin/rros-a11y"
# instalador: un NVMe con Windows (EFI, MSR, NTFS de 60 GiB) y ~40 GiB libres; sudo y rros-install falsos
N=$R/sys/devices/pci/nvme0n1
mkdir -p "$N/device" "$T/run"
echo 209715200 > "$N/size"; echo 259:0 > "$N/dev"; echo "WD Blue SN570" > "$N/device/model"
echo E:ID_PART_TABLE_TYPE=gpt > "$R/run/udev/data/b259:0"
i=1
for p in "1 100 vfat c12a7328-f81f-11d2-ba4b-00a0c93ec93b" "101 16 - e3c9e316-0b5c-4db8-817d-f92df00215ae" "117 61440 ntfs ebd0a0a2-b9e5-4433-87c0-68b6b72699c7"; do
	read -r st sz fs ty <<<"$p"
	mkdir -p "$N/nvme0n1p$i"
	echo 1 > "$N/nvme0n1p$i/partition"; echo $((st * 2048)) > "$N/nvme0n1p$i/start"; echo $((sz * 2048)) > "$N/nvme0n1p$i/size"
	echo "259:$i" > "$N/nvme0n1p$i/dev"
	{ [ "$fs" = - ] || echo "E:ID_FS_TYPE=$fs"; echo "E:ID_PART_ENTRY_TYPE=$ty"; } > "$R/run/udev/data/b259:$i"
	i=$((i + 1))
done
ln -s ../devices/pci/nvme0n1 "$R/sys/block/nvme0n1"
# shellcheck disable=SC2016  # contenido literal de los programas falsos
printf '#!/bin/sh\n[ "$1" = -n ] && shift\nexec "$@"\n' > "$T/bin/sudo"
cat > "$T/bin/rros-install" <<FAKE
#!/bin/sh
printf '%s\\n' "\$@" > "$T/install-args.txt"
prog="" pw=""
while [ \$# -gt 0 ]; do case \$1 in --progress-file) prog=\$2; shift ;; --password-file) pw=\$2; shift ;; esac; shift; done
cat "\$pw" > "$T/install-pw.txt"
for s in "1/7 Comprobando el sistema a copiar" "4/7 Copiando el sistema (40 %)" "7/7 Listo"; do echo "==> \$s"; echo "\$s" > "\$prog"; sleep 0.5; done
FAKE
chmod +x "$T/bin/sudo" "$T/bin/rros-install"
export RROS_PROFILE_DIR=$PWD/profiles RROS_PROFILE_FILE=$T/profile RROS_USER_CONF=$T/u.toml \
	RROS_SYSTEM_CONF=$T/s.toml XDG_CONFIG_HOME=$T/xdg RROS_THEMES_DIR=$T/themes RROS_GUI_DIR=$PWD/gui \
	RROS_APPS_CATALOG=$PWD/apps/catalog.toml PATH="$T/bin:$PATH" RROS_ROOT=$R \
	RROS_TEMPLATES_DIR=$PWD/templates HOME=$T/home RROS_LIVE=1 XDG_RUNTIME_DIR=$T/run
build/rros-settings web --no-open --port 18769 --token tk --timeout 300 > "$T/server.log" 2>&1 &
sleep 0.8
RROS_TEST_DIR=$T PORT=18769 TOKEN=tk node tests/gui_test.js
PWPATH=$PWPATH node tests/gui_demo_test.js
PWPATH=$PWPATH node tests/preview_test.js
