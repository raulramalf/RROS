#!/usr/bin/env bash
# Arranca una ISO de Lily en QEMU con RAM acotada al perfil.
# uso: run-qemu.sh PERFIL [--uefi]
# shellcheck source=common.sh
. "$(dirname "$0")/common.sh"

profile=${1:?uso: $0 PERFIL [--uefi]}
check_profile "$profile"
need_cmd qemu-system-x86_64
iso=$ROOT_DIR/build/rros-$profile.iso
[ -f "$iso" ] || die "no existe $iso"

case "$profile" in
	lite) ram=1024 cpus=1 ;;
	standard) ram=4096 cpus=2 ;;
	pro) ram=8192 cpus=4 ;;
esac

args=(-m "$ram" -smp "$cpus" -cdrom "$iso" -boot d -vga virtio -display "${QEMU_DISPLAY:-gtk}")
[ -w /dev/kvm ] && args+=(-enable-kvm -cpu host)
if [ "${2:-}" = "--uefi" ]; then
	fw=/usr/share/OVMF/OVMF_CODE.fd
	[ -f "$fw" ] || die "falta OVMF ($fw)"
	args+=(-bios "$fw")
fi
exec qemu-system-x86_64 "${args[@]}"
