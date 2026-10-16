#!/usr/bin/env bash
# Genera build/gui-demo.html: la GUI de ajustes con un servidor simulado, para verla sin instalar nada.
# El estado inicial (claves, temas, paletas) sale de `rros-settings state` con una configuración limpia.
# shellcheck source=common.sh
. "$(dirname "$0")/common.sh"

[ -x "$RROS/rros-settings" ] || die "compila primero con 'make'"
out=$RROS/gui-demo.html
tmp=$(mktemp -d)
trap 'rm -rf "${tmp:?}"' EXIT

RROS_PROFILE_DIR="$ROOT_DIR/profiles" RROS_PROFILE_FILE="$tmp/profile" RROS_USER_CONF="$tmp/user.toml" \
	RROS_SYSTEM_CONF="$tmp/sys.toml" RROS_THEMES_DIR="$tmp/themes" XDG_CONFIG_HOME="$tmp/xdg" \
	"$RROS/rros-settings" state > "$tmp/state.json"

# catálogo de aplicaciones: sin nada instalado (dpkg-query y flatpak simulados), perfil standard
mkdir -p "$tmp/bin"
printf '#!/bin/sh\nexit 1\n' > "$tmp/bin/dpkg-query"
printf '#!/bin/sh\nexit 1\n' > "$tmp/bin/flatpak"
chmod +x "$tmp/bin/dpkg-query" "$tmp/bin/flatpak"
PATH="$tmp/bin:$PATH" RROS_PROFILE_DIR="$ROOT_DIR/profiles" RROS_PROFILE_FILE="$tmp/profile" \
	RROS_APPS_CATALOG="$ROOT_DIR/apps/catalog.toml" "$RROS/rros-apps" list --json > "$tmp/apps.json"

# seguridad y dispositivos: las herramientas reales sobre un portátil de ejemplo (RROS_ROOT)
R=$tmp/root
mk() { mkdir -p "$(dirname "$R/$1")"; printf '%s\n' "$2" > "$R/$1"; }
mk sys/devices/nvme/nvme0n1/size 1000215216; mk sys/devices/nvme/nvme0n1/removable 0
mk sys/devices/nvme/nvme0n1/queue/rotational 0; mk sys/devices/nvme/nvme0n1/device/model "Samsung SSD 980"
for p in 1:1048576:259:1 2:999000000:259:2; do
	IFS=: read -r n sz ma mi <<<"$p"
	mk "sys/devices/nvme/nvme0n1/nvme0n1p$n/partition" "$n"; mk "sys/devices/nvme/nvme0n1/nvme0n1p$n/size" "$sz"
	mk "sys/devices/nvme/nvme0n1/nvme0n1p$n/dev" "$ma:$mi"
done
mk sys/devices/usb1/1-2/sdb/size 62521344; mk sys/devices/usb1/1-2/sdb/removable 1
mk sys/devices/usb1/1-2/sdb/device/vendor Kingston; mk sys/devices/usb1/1-2/sdb/device/model "DataTraveler 3.0"
mk sys/devices/usb1/1-2/sdb/sdb1/partition 1; mk sys/devices/usb1/1-2/sdb/sdb1/size 62519296; mk sys/devices/usb1/1-2/sdb/sdb1/dev 8:17
mkdir -p "$R/sys/block"; ln -s ../devices/nvme/nvme0n1 "$R/sys/block/nvme0n1"; ln -s ../devices/usb1/1-2/sdb "$R/sys/block/sdb"
mk run/udev/data/b259:2 "E:ID_FS_TYPE=ext4"; mk run/udev/data/b259:1 "E:ID_FS_TYPE=vfat"
mk run/udev/data/b8:17 "$(printf 'E:ID_FS_TYPE=vfat\nE:ID_FS_LABEL=FOTOS')"
mk proc/self/mounts "$(printf '/dev/nvme0n1p2 / ext4 rw 0 0\n/dev/nvme0n1p1 /boot/efi vfat rw 0 0\n/dev/sdb1 /media/rros/FOTOS vfat rw 0 0')"
mk sys/bus/usb/devices/1-1/idVendor 046d; mk sys/bus/usb/devices/1-1/idProduct c52b; mk sys/bus/usb/devices/1-1/manufacturer Logitech
mk sys/bus/usb/devices/1-1/product "USB Receiver"; mk sys/bus/usb/devices/1-1/speed 12
mk sys/class/net/wlp2s0/device/vendor 0x8086; mk sys/class/net/wlp2s0/wireless/x ""; mk sys/class/net/wlp2s0/operstate up
mk sys/class/net/wlp2s0/address "3c:a9:f4:12:34:56"
mk sys/class/rfkill/rfkill0/type wlan; mk sys/class/rfkill/rfkill0/name phy0; mk sys/class/rfkill/rfkill0/soft 0
mk sys/class/rfkill/rfkill1/type bluetooth; mk sys/class/rfkill/rfkill1/name hci0; mk sys/class/rfkill/rfkill1/soft 1
mk proc/asound/cards " 0 [PCH            ]: HDA-Intel - HDA Intel PCH"
mk sys/class/drm/card0-eDP-1/status connected; mk sys/class/drm/card0-eDP-1/modes 1920x1080
mk sys/class/power_supply/BAT0/type Battery; mk sys/class/power_supply/BAT0/capacity 87; mk sys/class/power_supply/BAT0/status Discharging
mk sys/class/power_supply/BAT0/energy_full 45000000; mk sys/class/power_supply/BAT0/energy_full_design 50000000
mk sys/class/power_supply/AC/type Mains; mk sys/class/power_supply/AC/online 0
mk proc/bus/input/devices "$(printf 'N: Name="AT Translated Set 2 keyboard"\nH: Handlers=sysrq kbd event0\n\nN: Name="SYNA2B46:00 Touchpad"\nH: Handlers=mouse1 event7')"
mk sys/class/video4linux/video0/name "Integrated Camera"; mk sys/class/video4linux/video0/index 0
mk sys/bus/pci/devices/0000:00:02.0/class 0x030000; mk sys/bus/pci/devices/0000:00:02.0/vendor 0x8086
mk sys/bus/pci/devices/0000:00:02.0/device 0x9a49; mk sys/bus/pci/devices/0000:00:02.0/driver/x ""
mk usr/share/misc/pci.ids "$(printf '8086  Intel Corporation\n\t9a49  TigerLake-LP GT2 [Iris Xe Graphics]')"
mk proc/cmdline quiet; mk sys/module/apparmor/parameters/enabled Y; mk etc/passwd "root:x:0:0:root:/root:/bin/bash"
mk etc/nftables.conf "table inet rros { chain input { policy drop; } }"
mk etc/apt/apt.conf.d/20auto-upgrades 'APT::Periodic::Unattended-Upgrade "1";'
mk etc/sudoers.d/rros "Defaults timestamp_timeout=5"; mk proc/sys/fs/suid_dumpable 0
mk usr/bin/unattended-upgrade ""
while read -r k v; do mk "proc/sys/${k//.//}" "$v"; done <<'EOF'
kernel.kptr_restrict 2
kernel.dmesg_restrict 1
kernel.unprivileged_bpf_disabled 2
kernel.yama.ptrace_scope 1
fs.protected_symlinks 1
fs.protected_hardlinks 1
fs.protected_fifos 2
fs.protected_regular 2
net.ipv4.tcp_syncookies 1
EOF
# shellcheck disable=SC2016  # contenido literal del systemctl falso
printf '#!/bin/sh\n[ "$3" = nftables.service ]\n' > "$tmp/bin/systemctl"
chmod +x "$tmp/bin/systemctl"
PATH="$tmp/bin:$PATH" RROS_ROOT=$R RROS_PROFILE_DIR="$ROOT_DIR/profiles" RROS_PROFILE_FILE="$tmp/profile" \
	RROS_USER_CONF="$tmp/user.toml" RROS_SYSTEM_CONF="$tmp/sys.toml" "$RROS/rros-security" auditoria --json > "$tmp/security.json"
RROS_ROOT=$R "$RROS/rros-devices" lista --json > "$tmp/devices.json"
mkdir -p "$tmp/home/Documentos"
HOME=$tmp/home XDG_CONFIG_HOME=$tmp/home/.config RROS_TEMPLATES_DIR="$ROOT_DIR/templates" "$RROS/rros-new" lista --json |
	sed "s#$tmp/home#/home/rros#g" > "$tmp/templates.json"

# awk -v interpretaría las barras invertidas del JSON: se lee desde archivo
awk -v statefile="$tmp/state.json" -v appsfile="$tmp/apps.json" -v mock="$ROOT_DIR/gui/mock.js" \
	-v secfile="$tmp/security.json" -v devfile="$tmp/devices.json" -v tplfile="$tmp/templates.json" '
	!done && /^<script>$/ {
		print "<script>\nwindow.RROS_MOCK_STATE ="
		while ((getline line < statefile) > 0) print line
		print ";\n</script>"
		print "<script>\nwindow.RROS_MOCK_APPS ="
		while ((getline line < appsfile) > 0) print line
		print ";\nwindow.RROS_MOCK_SECURITY ="
		while ((getline line < secfile) > 0) print line
		print ";\nwindow.RROS_MOCK_DEVICES ="
		while ((getline line < devfile) > 0) print line
		print ";\nwindow.RROS_MOCK_TEMPLATES ="
		while ((getline line < tplfile) > 0) print line
		print ";\n</script>"
		print "<script>"
		while ((getline line < mock) > 0) print line
		print "</script>"
		done = 1
	}
	{ print }
' "$ROOT_DIR/gui/index.html" > "$out"
log "demo generada: $out ($(wc -c < "$out") bytes)"
