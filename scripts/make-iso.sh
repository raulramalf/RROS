#!/usr/bin/env bash
# Empaqueta un rootfs en una ISO arrancable (squashfs + GRUB). Rootfs Ubuntu con casper.
# Arranca en BIOS y en UEFI, desde CD o memoria USB, y con Secure Boot activado: en UEFI usa el shim
# firmado por Microsoft y el GRUB firmado por Canonical del propio rootfs (como la ISO de Ubuntu).
# uso: make-iso.sh PERFIL
# shellcheck source=common.sh
. "$(dirname "$0")/common.sh"

profile=${1:?uso: $0 PERFIL}
check_profile "$profile"
need_root
need_cmd mksquashfs xorriso grub-mkimage mformat mmd mcopy
rootfs=$ROOT_DIR/build/rootfs-ubuntu-$profile
iso_dir=$ROOT_DIR/build/iso-$profile
iso=${ISO_OUT:-$ROOT_DIR/build/rros-$profile.iso}

[ -d "$rootfs" ] || die "no existe $rootfs (ejecuta build-rootfs-ubuntu.sh)"
vmlinuz=$(find "$rootfs/boot" -maxdepth 1 -name 'vmlinuz-*' | sort -V | tail -1)
initrd=$(find "$rootfs/boot" -maxdepth 1 -name 'initrd.img-*' | sort -V | tail -1)
if [ -z "$vmlinuz" ] || [ -z "$initrd" ]; then
	die "el rootfs no tiene kernel/initrd en /boot"
fi

# REUSE_SQUASHFS=1 conserva el squashfs ya hecho (para probar solo el arranque de la ISO)
if [ "${REUSE_SQUASHFS:-0}" = 1 ] && [ -s "$iso_dir/casper/filesystem.squashfs" ]; then
	log "se reutiliza el squashfs"
	find "$iso_dir" -mindepth 1 -maxdepth 1 ! -name casper -exec rm -rf {} +
else
	rm -rf "${iso_dir:?}"
fi
mkdir -p "$iso_dir/casper" "$iso_dir/boot/grub"
cp "$vmlinuz" "$iso_dir/casper/vmlinuz"
cp "$initrd" "$iso_dir/casper/initrd"

if [ ! -s "$iso_dir/casper/filesystem.squashfs" ]; then
	log "squashfs (zstd)"
	mksquashfs "$rootfs" "$iso_dir/casper/filesystem.squashfs" -comp zstd -Xcompression-level "${SQUASH_LEVEL:-15}" \
		-e boot/vmlinuz-* boot/initrd.img-* -noappend
	# tamaño sin comprimir: el instalador lo usa para mostrar el porcentaje de la copia
	du -sx --block-size=1 "$rootfs" | cut -f1 > "$iso_dir/casper/filesystem.size"
fi

# EXTRA_CMDLINE añade parámetros a las entradas (ISOs de prueba: "console=tty0 console=ttyS0 rros.serialshell")
extra=${EXTRA_CMDLINE:-}
[[ $extra =~ ^[A-Za-z0-9_=,.:\ /-]*$ ]] || die "EXTRA_CMDLINE con caracteres no permitidos"
cat > "$iso_dir/boot/grub/grub.cfg" <<GRUB
set timeout=5
set color_normal=cyan/black
set color_highlight=black/cyan
menuentry "Probar o instalar RROS Aurora ($profile)" {
	linux /casper/vmlinuz boot=casper quiet splash $extra ---
	initrd /casper/initrd
}
menuentry "RROS Aurora ($profile) - modo seguro (gráficos básicos, si la pantalla se queda en negro)" {
	linux /casper/vmlinuz boot=casper quiet nomodeset $extra ---
	initrd /casper/initrd
}
if [ "\$grub_platform" = "efi" ]; then
	menuentry "Configuración del firmware (UEFI)" {
		fwsetup
	}
fi
GRUB
# el GRUB firmado busca este archivo para saber en qué disco está la ISO; vacío, porque si tiene texto
# casper intenta añadir la ISO como fuente de apt (y no es un CD de paquetes)
mkdir -p "$iso_dir/.disk"
: > "$iso_dir/.disk/info"

# BIOS: GRUB de i386-pc (los módulos se cargan de /boot/grub/i386-pc)
grub_pc=/usr/lib/grub/i386-pc
[ -f "$grub_pc/boot_hybrid.img" ] || die "falta grub-pc-bin en este equipo"
mkdir -p "$iso_dir/boot/grub/i386-pc"
cp "$grub_pc"/*.mod "$grub_pc"/*.lst "$iso_dir/boot/grub/i386-pc/"
grub-mkimage -O i386-pc-eltorito -p /boot/grub -o "$iso_dir/boot/grub/i386-pc/eltorito.img" biosdisk iso9660

# UEFI: shim (firmado por Microsoft) -> GRUB para CD (firmado por Canonical) -> kernel firmado
shim=$rootfs/usr/lib/shim/shimx64.efi.signed.latest
[ -f "$shim" ] || shim=$rootfs/usr/lib/shim/shimx64.efi.signed.previous
gcd=$rootfs/usr/lib/grub/x86_64-efi-signed/gcdx64.efi.signed
mm=$rootfs/usr/lib/shim/mmx64.efi
for f in "$shim" "$gcd" "$mm"; do [ -f "$f" ] || die "falta $f en el rootfs (shim-signed, grub-efi-amd64-signed)"; done
mkdir -p "$iso_dir/EFI/BOOT"
cp "$shim" "$iso_dir/EFI/BOOT/BOOTX64.EFI"
cp "$gcd" "$iso_dir/EFI/BOOT/grubx64.efi"
cp "$mm" "$iso_dir/EFI/BOOT/mmx64.efi"
efi_img=$ROOT_DIR/build/efi-$profile.img
rm -f "$efi_img"
truncate -s 8M "$efi_img"
mformat -i "$efi_img" -v RROS_EFI ::
mmd -i "$efi_img" ::/EFI ::/EFI/BOOT
mcopy -i "$efi_img" "$iso_dir/EFI/BOOT/BOOTX64.EFI" "$iso_dir/EFI/BOOT/grubx64.efi" "$iso_dir/EFI/BOOT/mmx64.efi" ::/EFI/BOOT/

log "ISO -> $iso"
# la misma disposición que la ISO de Ubuntu 24.04: híbrida (CD y USB), MBR + GPT con la partición EFI aparte
rm -f "$iso"
xorriso -as mkisofs -r -J -joliet-long -iso-level 3 -V "RROS_${profile^^}" \
	--grub2-mbr "$grub_pc/boot_hybrid.img" --protective-msdos-label -partition_cyl_align off \
	-partition_offset 16 --mbr-force-bootable \
	-append_partition 2 28732ac11ff8d211ba4b00a0c93ec93b "$efi_img" -appended_part_as_gpt \
	-iso_mbr_part_type a2a0d0ebe5b9334487c068b6b72699c7 \
	-c /boot.catalog -b /boot/grub/i386-pc/eltorito.img -no-emul-boot -boot-load-size 4 -boot-info-table --grub2-boot-info \
	-eltorito-alt-boot -e '--interval:appended_partition_2:all::' -no-emul-boot \
	-o "$iso" "$iso_dir"
ls -lh "$iso"
size_mb=$(( $(stat -c %s "$iso") / 1048576 ))
echo "tamaño ISO: ${size_mb} MB"
