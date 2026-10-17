#!/usr/bin/env bash
# Construye la ISO de RROS sobre Arch Linux con archiso (mkarchiso). Requiere root y un sistema Arch con
# los paquetes archiso y base-devel (en CI: el contenedor archlinux, ver .github/workflows/arch.yml).
# uso: build-iso-arch.sh PERFIL
# Variables: EXTRA_CMDLINE  parámetros extra del kernel en el menú (ISOs de prueba)
#            ISO_OUT        ruta de la ISO (por defecto build/rros-arch-PERFIL.iso)
# Del perfil «releng» de archiso solo se toma el arranque (syslinux, systemd-boot, GRUB) y el initramfs del
# live; los paquetes, la capa y la identidad son de RROS. La identidad la aplica el hook de pacman
# zz-rros.hook al instalar los paquetes (os-release pertenece al paquete «filesystem»).
# shellcheck source=common.sh
. "$(dirname "$0")/common.sh"

profile=${1:?uso: $0 PERFIL}
check_profile "$profile"
need_root
need_cmd mkarchiso pacman
[ -x "$RROS/rros-config" ] || die "compila primero con 'make'"
releng=/usr/share/archiso/configs/releng
[ -d "$releng" ] || die "falta archiso ($releng)"
extra=${EXTRA_CMDLINE:-}
[[ $extra =~ ^[A-Za-z0-9_=,.:\ /-]*$ ]] || die "EXTRA_CMDLINE con caracteres no permitidos"
P=$ROOT_DIR/build/archiso-$profile
work=$ROOT_DIR/build/archiso-work-$profile
iso=${ISO_OUT:-$ROOT_DIR/build/rros-arch-$profile.iso}

rm -rf "${P:?}" "${work:?}"
mkdir -p "$P/airootfs"
for f in profiledef.sh pacman.conf bootstrap_packages.x86_64 syslinux efiboot grub; do
	[ -e "$releng/$f" ] && cp -a "$releng/$f" "$P/"
done

# lo imprescindible del live de releng: su initramfs (hooks de archiso) y el llavero de pacman;
# nada del instalador de Arch (root sin contraseña, mensajes, ssh, cloud-init...)
keep() {
	[ -e "$releng/airootfs/$1" ] || [ -L "$releng/airootfs/$1" ] || return 0
	mkdir -p "$P/airootfs/$(dirname "$1")"
	cp -a "$releng/airootfs/$1" "$P/airootfs/$1"
}
keep etc/mkinitcpio.conf.d/archiso.conf
keep etc/mkinitcpio.d/linux.preset
keep etc/systemd/journald.conf.d/volatile-storage.conf
keep etc/systemd/system/pacman-init.service
keep etc/systemd/system/etc-pacman.d-gnupg.mount
for l in "$releng"/airootfs/etc/systemd/system/*.wants/*; do
	case $(readlink "$l") in *pacman-init.service | *etc-pacman.d-gnupg.mount) keep "${l#"$releng"/airootfs/}" ;; esac
done

# paquetes: base de RROS + perfil + lo que necesita el live
{
	read_list "$ROOT_DIR/packages/arch-base.list" "$ROOT_DIR/packages/arch-$profile.list" "$ROOT_DIR/packages/arch-live.list" | tr ' ' '\n'
	# lo que usan las entradas del menú de releng (memtest, consola UEFI)
	echo memtest86+
	echo memtest86+-efi
	echo edk2-shell
} | sed '/^$/d' | sort -u > "$P/packages.x86_64"

# herramientas y capa de RROS
make -C "$ROOT_DIR" install DESTDIR="$P/airootfs" PREFIX=/usr >/dev/null
"$ROOT_DIR/scripts/apply-overlay.sh" "$P/airootfs" "$profile" arch

# nombre, etiqueta y permisos (se añaden al final: sustituyen a los de releng)
cat >> "$P/profiledef.sh" <<EOF

# --- RROS ---
iso_name="rros-$profile"
iso_label="RROS_${profile^^}"
iso_publisher="RROS <https://github.com/raulramalf/RROS>"
iso_application="RROS Aurora ($profile): probar e instalar"
airootfs_image_type="squashfs"
airootfs_image_tool_options=('-comp' 'zstd' '-Xcompression-level' '${SQUASH_LEVEL:-15}' '-b' '1M')
file_permissions=(
  ["/etc/sudoers.d/10-rros-wheel"]="0:0:440"
  ["/usr/libexec/rros-arch-setup"]="0:0:755"
  ["/usr/libexec/rros-live-user"]="0:0:755"
)
EOF

# menú de arranque: nombre de RROS y parámetros
find "$P/syslinux" "$P/efiboot" "$P/grub" -type f \( -name '*.cfg' -o -name '*.conf' \) 2>/dev/null | while read -r f; do
	sed -i -e "s/Arch Linux install medium/RROS Aurora ($profile)/g" -e "s/Arch Linux/RROS/g" \
		-e "s|archisosearchuuid=%ARCHISO_UUID%|& lsm=landlock,lockdown,yama,integrity,apparmor,bpf quiet splash ${extra}|g" "$f"
done

log "mkarchiso -> $iso"
mkarchiso -v -w "$work" -o "$ROOT_DIR/build/iso-out-$profile" "$P"
mv "$ROOT_DIR"/build/iso-out-"$profile"/rros-"$profile"-*.iso "$iso"
rm -rf "${work:?}" "$ROOT_DIR/build/iso-out-$profile"
ls -lh "$iso"
