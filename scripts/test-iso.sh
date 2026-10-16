#!/usr/bin/env bash
# Arranca la ISO de RROS en QEMU y comprueba que el sistema funciona.
#   test-iso.sh PERFIL selftest     arranque por consola serie con el autotest (rros.selftest)
#   test-iso.sh PERFIL screenshot   arranca con GRUB y guarda una captura del escritorio
# Variables: RAM_MB (por defecto 2048), WAIT (segundos antes de la primera captura, 300),
#            COUNT (nº de capturas, 1) e INTERVAL (segundos entre capturas, 60), OUT (carpeta)
# shellcheck source=common.sh
. "$(dirname "$0")/common.sh"

profile=${1:?uso: $0 PERFIL selftest|screenshot}
mode=${2:-selftest}
check_profile "$profile"
need_cmd qemu-system-x86_64
iso=$ROOT_DIR/build/rros-$profile.iso
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
		timeout "${TIMEOUT:-1500}" qemu-system-x86_64 "${accel[@]}" -m "$ram" -smp 2 -nographic -no-reboot \
			-kernel "$iso_dir/casper/vmlinuz" -initrd "$iso_dir/casper/initrd" \
			-append "boot=casper quiet console=ttyS0 rros.selftest loglevel=3" \
			-cdrom "$iso" > "$out/serial.log" 2>&1 || true
		grep -a -E '^\[(ok|FALLO)\]|=== ' "$out/serial.log" | tr -d '\r' || true
		if grep -aq RROS_SELFTEST_OK "$out/serial.log"; then
			log "AUTOTEST CORRECTO"
		else
			die "el autotest falló o no terminó (mira $out/serial.log)"
		fi
		;;
	screenshot)
		need_cmd python3
		sock=$out/monitor.sock
		rm -f "$sock"
		log "arranque gráfico con GRUB; captura a los ${WAIT:-300} s"
		qemu-system-x86_64 "${accel[@]}" -m "$ram" -smp 2 -cdrom "$iso" -boot d -vga std -display none \
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
