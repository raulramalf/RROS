#!/usr/bin/env bash
# Arranca Lily en QEMU (en Linux), con ventana, aceleración y portapapeles compartido.
#   vm-lily.sh lily-standard.vmdk     la máquina virtual ya instalada (usuario y contraseña «lily»)
#   vm-lily.sh lily-pro.iso           la ISO: sistema en vivo e instalador (crea al lado un disco lily-pro.disco.qcow2)
# Variables: RAM_MB (por defecto, la de la edición: lite 3072, standard 4096, pro 6144), CPUS (2), UEFI=1
set -euo pipefail

img=${1:?uso: $0 lily-EDICIÓN.vmdk | lily-EDICIÓN.iso}
[ -r "$img" ] || { echo "vm-lily: no encuentro $img" >&2; exit 1; }
command -v qemu-system-x86_64 >/dev/null || { echo "vm-lily: instala QEMU (paquete qemu-desktop o qemu-system-x86)" >&2; exit 1; }

case $img in
*pro*) ram=6144 ;;
*standard*) ram=4096 ;;
*) ram=3072 ;;
esac
ram=${RAM_MB:-$ram}

# shellcheck disable=SC2054  # las comas son de las opciones de QEMU
args=(-m "$ram" -smp "${CPUS:-2}" -vga virtio -display gtk,gl=on -nic user,model=virtio-net-pci
	-device virtio-serial -chardev spicevmc,id=vda,name=vdagent -device virtserialport,chardev=vda,name=com.redhat.spice.0
	-audiodev pa,id=snd -device intel-hda -device hda-duplex,audiodev=snd)
if [ -w /dev/kvm ]; then
	args+=(-enable-kvm -cpu host)
else
	echo "vm-lily: sin KVM irá muy lento (activa la virtualización en la BIOS y entra en el grupo kvm)" >&2
fi
if [ "${UEFI:-0}" = 1 ]; then
	code=$(find /usr/share/OVMF /usr/share/edk2/x64 -maxdepth 1 -name 'OVMF_CODE*.fd' 2>/dev/null | sort | head -1)
	[ -n "$code" ] || { echo "vm-lily: falta OVMF (paquete ovmf o edk2-ovmf)" >&2; exit 1; }
	args+=(-bios "$code")
fi

case $img in
*.iso)
	disk=${img%.iso}.disco.qcow2
	[ -e "$disk" ] || qemu-img create -f qcow2 "$disk" 40G >/dev/null
	echo "vm-lily: ISO en vivo; el disco para instalar es $disk"
	exec qemu-system-x86_64 "${args[@]}" -cdrom "$img" -drive "file=$disk,if=virtio,format=qcow2" -boot d ;;
*.vmdk) exec qemu-system-x86_64 "${args[@]}" -drive "file=$img,if=virtio,format=vmdk" ;;
*) exec qemu-system-x86_64 "${args[@]}" -drive "file=$img,if=virtio" ;;
esac
