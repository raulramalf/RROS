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
[ -x "$ROOT_DIR/build/app/rros-ajustes" ] || die "compila RROS Ajustes con 'make app' (necesita cmake, qt6-base y qt6-declarative)"
[ -x "$ROOT_DIR/build/shell/rros-shell" ] || die "compila RROS Shell con 'make shell SHELL_FULL=ON' (necesita además layer-shell-qt, kwindowsystem y kglobalaccel)"
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

# nombre, etiqueta y permisos (se añaden al final: sustituyen a los de releng). mkarchiso copia airootfs sin
# conservar los permisos (cp --no-preserve=mode): todo lo ejecutable (programas de RROS, scripts) tiene que ir
# en file_permissions o se queda sin permiso de ejecución
{
	cat <<EOF

# --- RROS ---
iso_name="rros-$profile"
iso_label="RROS_${profile^^}"
iso_publisher="RROS <https://github.com/raulramalf/RROS>"
iso_application="RROS 2.0 ($profile): probar e instalar"
airootfs_image_type="squashfs"
airootfs_image_tool_options=('-comp' 'zstd' '-Xcompression-level' '${SQUASH_LEVEL:-15}' '-b' '1M')
file_permissions=(
  ["/etc/sudoers.d/10-rros-wheel"]="0:0:440"
EOF
	(cd "$P/airootfs" && find . -type f -perm -u+x -printf '  ["/%P"]="0:0:755"\n')
	echo ")"
} >> "$P/profiledef.sh"
log "$(grep -c '"0:0:755"' "$P/profiledef.sh") archivos ejecutables en file_permissions"

# menú de arranque, en español: RROS, «modo seguro» (sin el controlador gráfico, para pantallas negras) y
# lector de pantalla (accessibility=on: rros-live-user activa Orca). cow_spacesize=50%: lo que se escribe en el
# live (aplicaciones de la Tienda incluidas) puede ocupar hasta la mitad de la RAM, no solo 256 MB. Las entradas nuevas copian la primera de
# releng para no depender del formato de cada versión de archiso.
seguro="RROS 2.0 ($profile) - modo seguro (gráficos básicos)"
ayuda_seguro="Sin el controlador de la tarjeta gráfica: si la pantalla se queda negra o congelada."
f=$P/syslinux/archiso_sys-linux.cfg
if [ -f "$f" ]; then
	awk -v label="$seguro" -v help="$ayuda_seguro" '
		!done && /^LABEL / { inb = 1 }
		inb { b[++nb] = $0 }
		{ print }
		inb && /^APPEND / {
			inb = 0; done = 1; print ""
			for (i = 1; i <= nb; i++) {
				l = b[i]
				if (l ~ /^LABEL /) l = "LABEL rrosseguro"
				else if (l ~ /^MENU LABEL /) l = "MENU LABEL " label
				else if (l ~ /^APPEND /) l = l " nomodeset"
				else if (l ~ /^TEXT HELP/) { print l; print help; skip = 1; continue }
				else if (l ~ /^ENDTEXT/) skip = 0
				else if (skip) continue
				print l
			}
		}' "$f" > "$f.new" && mv "$f.new" "$f"
fi
first=$(find "$P/efiboot/loader/entries" -name '*.conf' ! -name '*speech*' ! -name '*memtest*' ! -name '*shell*' 2>/dev/null | sort | head -1)
if [ -n "$first" ]; then
	sed -e "s/^title .*/title    $seguro/" -e 's/^\(options .*\)/\1 nomodeset/' -e 's/^sort-key .*/sort-key 01s/' \
		"$first" > "$(dirname "$first")/01-archiso-seguro.conf"
fi
find "$P/syslinux" "$P/efiboot" "$P/grub" -type f \( -name '*.cfg' -o -name '*.conf' \) 2>/dev/null | while read -r f; do
	sed -i -e "s/^Boot the .* with speakup screen reader\./Arranca RROS con el lector de pantalla Orca./" \
		-e "s/^It allows you to install .* with speech feedback\./Puedes probarlo e instalarlo con voz./" \
		-e "s/Arch Linux install medium/RROS 2.0 ($profile)/g" \
		-e "s/ with speakup screen reader/ con lector de pantalla/g" -e "s/ with ^speech/ con ^lector de pantalla/g" \
		-e "s/ with speech$/ con lector de pantalla/" \
		-e "s/^Boot the .*/Arranca RROS: pruébalo sin tocar el disco y, si te gusta, instálalo./" \
		-e "/^It allows you to install /d" \
		-e "s/Run Memtest86+ (RAM test)/Comprobar la memoria (Memtest86+)/g" \
		-e "s/Boot existing OS/Arrancar el sistema del disco/g" \
		-e "s/^Boot an existing operating system\./Arranca el sistema que ya está instalado en el disco./" \
		-e "s/Hardware Information (HDT)/Información del equipo (HDT)/g" \
		-e "s/^MENU LABEL Reboot$/MENU LABEL Reiniciar/" -e "s/^Reboot computer\./Reinicia el equipo./" \
		-e "s/^MENU LABEL Power Off$/MENU LABEL Apagar/" -e "s/^Power off computer\./Apaga el equipo./" \
		-e "s/^Press TAB to edit the disk and partition number to boot\./Pulsa TAB para elegir otro disco o partición./" \
		-e "s/^The computer's firmware must support APM\./El firmware del equipo tiene que admitir APM./" \
		-e "s/'System shutdown'/'Apagar'/" -e "s/'System restart'/'Reiniciar'/" \
		-e "s/'UEFI Firmware Settings'/'Ajustes del firmware (UEFI)'/" \
		-e "s/System shutting down\.\.\./Apagando.../" -e "s/System rebooting\.\.\./Reiniciando.../" \
		-e "s/Arch Linux/RROS/g" \
		-e "s|archisosearchuuid=%ARCHISO_UUID%|& lsm=landlock,lockdown,yama,integrity,apparmor,bpf cow_spacesize=50% quiet splash${extra:+ $extra}|g" "$f"
done
# fondo del menú BIOS: el de RROS, generado con los colores del tema (vesamenu: 640x480)
if [ -f "$P/syslinux/splash.png" ]; then
	HOME=$P/tmp-home XDG_DATA_HOME=$P/tmp-home "$RROS/rros-theme" -p "$profile" wallpaper --style aurora --size 640x480 \
		--out "$P/syslinux/splash.png" >/dev/null || log "aviso: no se pudo generar el fondo del menú"
	rm -rf "${P:?}/tmp-home"
fi

log "mkarchiso -> $iso"
mkarchiso -v -w "$work" -o "$ROOT_DIR/build/iso-out-$profile" "$P"
mv "$ROOT_DIR"/build/iso-out-"$profile"/rros-"$profile"-*.iso "$iso"
rm -rf "${work:?}" "$ROOT_DIR/build/iso-out-$profile"
ls -lh "$iso"
