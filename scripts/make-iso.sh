#!/usr/bin/env bash
# Empaqueta un rootfs en una ISO arrancable (squashfs + GRUB). Rootfs Ubuntu con casper.
# uso: make-iso.sh PERFIL
# shellcheck source=common.sh
. "$(dirname "$0")/common.sh"

profile=${1:?uso: $0 PERFIL}
check_profile "$profile"
need_root
need_cmd mksquashfs grub-mkrescue xorriso
rootfs=$ROOT_DIR/build/rootfs-ubuntu-$profile
iso_dir=$ROOT_DIR/build/iso-$profile
iso=$ROOT_DIR/build/rros-$profile.iso

[ -d "$rootfs" ] || die "no existe $rootfs (ejecuta build-rootfs-ubuntu.sh)"
vmlinuz=$(ls "$rootfs"/boot/vmlinuz-* | sort -V | tail -1)
initrd=$(ls "$rootfs"/boot/initrd.img-* | sort -V | tail -1)

rm -rf "${iso_dir:?}"
mkdir -p "$iso_dir/casper" "$iso_dir/boot/grub"
cp "$vmlinuz" "$iso_dir/casper/vmlinuz"
cp "$initrd" "$iso_dir/casper/initrd"

log "squashfs (zstd)"
mksquashfs "$rootfs" "$iso_dir/casper/filesystem.squashfs" -comp zstd -Xcompression-level 15 \
	-e boot/vmlinuz-* boot/initrd.img-* -noappend

cat > "$iso_dir/boot/grub/grub.cfg" <<GRUB
set timeout=3
set color_normal=cyan/black
set color_highlight=black/cyan
menuentry "RROS Aurora ($profile)" {
	linux /casper/vmlinuz boot=casper quiet splash ---
	initrd /casper/initrd
}
menuentry "RROS Aurora ($profile) - modo seguro" {
	linux /casper/vmlinuz boot=casper nomodeset ---
	initrd /casper/initrd
}
GRUB

log "ISO -> $iso"
grub-mkrescue -o "$iso" "$iso_dir" -- -volid "RROS_${profile^^}"
ls -lh "$iso"
size_mb=$(( $(stat -c %s "$iso") / 1048576 ))
echo "tamaño ISO: ${size_mb} MB"
