#!/usr/bin/env bash
# Arranca la ISO de RROS en QEMU y comprueba que el sistema funciona.
#   test-iso.sh PERFIL selftest     arranque por consola serie con el autotest (rros.selftest)
#   test-iso.sh PERFIL screenshot   arranca con GRUB y guarda una captura del escritorio
# Variables: RAM_MB (por defecto 2048), WAIT (segundos antes de la primera captura, 300),
#            COUNT (nº de capturas, 1) e INTERVAL (segundos entre capturas, 60), OUT (carpeta)
#   En screenshot: FIRMWARE=bios|uefi|secboot (secboot = UEFI con Secure Boot y las claves de Microsoft,
#   como un PC con Windows) y MEDIA=cd|usb (la ISO grabada en una memoria USB)
#   BASE=arch para la ISO de Arch (ISO=build/rros-arch-PERFIL.iso)
# shellcheck source=common.sh
. "$(dirname "$0")/common.sh"

profile=${1:?uso: $0 PERFIL selftest|screenshot}
mode=${2:-selftest}
check_profile "$profile"
need_cmd qemu-system-x86_64
iso=${ISO:-$ROOT_DIR/build/rros-$profile.iso}
iso_dir=$ROOT_DIR/build/iso-$profile
out=${OUT:-$ROOT_DIR/build/test-$profile}
ram=${RAM_MB:-2048}
[ -f "$iso" ] || die "no existe $iso (ejecuta make-iso.sh)"
mkdir -p "$out"

accel=()
[ -w /dev/kvm ] && accel=(-enable-kvm -cpu host)

case "$mode" in
	selftest)
		log "arranque por consola serie con autotest (RAM ${ram} MB); puede tardar varios minutos sin KVM"
		: > "$out/serial.log"
		if [ "${BASE:-ubuntu}" = arch ]; then
			# ISO de archiso: el kernel y el initramfs se sacan de la propia ISO
			need_cmd xorriso
			xorriso -osirrox on -indev "$iso" -extract /arch/boot/x86_64/vmlinuz-linux "$out/vmlinuz" \
				-extract /arch/boot/x86_64/initramfs-linux.img "$out/initrd" >/dev/null 2>&1 ||
				die "no se pudo sacar el kernel de $iso"
			kernel=$out/vmlinuz initrd=$out/initrd
			label=$(xorriso -indev "$iso" -pvd_info 2>/dev/null | sed -n 's/^Volume Id *: *//p' | tr -d "'")
			append="archisobasedir=arch archisolabel=${label:-RROS_${profile^^}} lsm=landlock,lockdown,yama,integrity,apparmor,bpf"
			append+=" console=ttyS0 rros.selftest loglevel=3"
		else
			kernel=$iso_dir/casper/vmlinuz initrd=$iso_dir/casper/initrd
			append="boot=casper quiet console=ttyS0 rros.selftest loglevel=3"
		fi
		qemu-system-x86_64 "${accel[@]}" -m "$ram" -smp 2 -nographic -no-reboot \
			-kernel "$kernel" -initrd "$initrd" -append "$append" \
			-cdrom "$iso" > "$out/serial.log" 2>&1 &
		qpid=$!
		# se corta en cuanto el autotest da su veredicto (el apagado del live puede tardar mucho sin KVM)
		waited=0
		while kill -0 "$qpid" 2>/dev/null && [ "$waited" -lt "${TIMEOUT:-1500}" ]; do
			grep -aq 'RROS_SELFTEST_\(OK\|FAIL\)' "$out/serial.log" && break
			sleep 5
			waited=$((waited + 5))
		done
		kill "$qpid" 2>/dev/null || true
		wait "$qpid" 2>/dev/null || true
		grep -a -E '^\[(ok|FALLO)\]|=== ' "$out/serial.log" | tr -d '\r' || true
		if grep -aq RROS_SELFTEST_OK "$out/serial.log"; then
			log "AUTOTEST CORRECTO"
		else
			echo "--- últimas líneas de la consola serie ---"
			tail -n 80 "$out/serial.log" | tr -d '\r'
			die "el autotest falló o no terminó (mira $out/serial.log)"
		fi
		;;
	screenshot)
		need_cmd python3
		sock=$out/monitor.sock
		rm -f "$sock"
		fwargs=()
		case ${FIRMWARE:-bios} in
		bios) ;;
		uefi)
			cp /usr/share/OVMF/OVMF_VARS_4M.fd "$out/vars.fd"
			fwargs=(-drive "if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd" -drive "if=pflash,format=raw,file=$out/vars.fd") ;;
		secboot)
			cp /usr/share/OVMF/OVMF_VARS_4M.ms.fd "$out/vars.fd"
			fwargs=(-machine "q35,smm=on" -global "driver=cfi.pflash01,property=secure,value=on"
				-drive "if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.secboot.fd" -drive "if=pflash,format=raw,file=$out/vars.fd") ;;
		*) die "FIRMWARE: bios, uefi o secboot" ;;
		esac
		if [ "${MEDIA:-cd}" = usb ]; then
			media=(-device qemu-xhci -drive "file=$iso,if=none,id=usb0,format=raw,readonly=on" -device "usb-storage,drive=usb0,bootindex=0")
		else
			media=(-cdrom "$iso" -boot d)
		fi
		log "arranque por el firmware (${FIRMWARE:-bios}, ${MEDIA:-cd}); captura a los ${WAIT:-300} s"
		qemu-system-x86_64 "${accel[@]}" "${fwargs[@]}" -m "$ram" -smp 2 "${media[@]}" -vga std -display none \
			-monitor "unix:$sock,server,nowait" -serial "file:$out/serial.log" -no-reboot &
		qpid=$!
		trap 'kill "$qpid" 2>/dev/null || true' EXIT
		sleep "${WAIT:-300}"
		for n in $(seq 1 "${COUNT:-1}"); do
			name=escritorio.png
			[ "${COUNT:-1}" -gt 1 ] && name=escritorio-$n.png
			python3 "$ROOT_DIR/scripts/qemu-shot.py" "$sock" "$out/$name"
			[ "$n" -lt "${COUNT:-1}" ] && sleep "${INTERVAL:-60}"
		done
		;;
	*) die "modo desconocido: $mode" ;;
esac
