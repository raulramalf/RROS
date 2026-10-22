#!/usr/bin/env bash
# Arranca la ISO de Lily en QEMU y comprueba que el sistema funciona.
#   test-iso.sh PERFIL selftest     arranque por consola serie con el autotest (rros.selftest)
#   test-iso.sh PERFIL install      instala en un disco virtual (rros.installtest), arranca desde él y pasa el autotest
#   test-iso.sh PERFIL screenshot   arranca con GRUB y guarda una captura del escritorio
# Variables: RAM_MB (por defecto 2048), TIMEOUT (segundos de cada arranque, 1500), DISK_GB (16), WAIT (segundos antes de la primera captura, 300),
#            COUNT (nº de capturas, 1) e INTERVAL (segundos entre capturas, 60), OUT (carpeta)
#   En screenshot: FIRMWARE=bios|uefi|secboot (secboot = UEFI con Secure Boot y las claves de Microsoft,
#   como un PC con Windows) y MEDIA=cd|usb (la ISO grabada en una memoria USB)
#   BASE=arch para la ISO de Arch (ISO=build/lily-PERFIL.iso)
# shellcheck source=common.sh
. "$(dirname "$0")/common.sh"

profile=${1:?uso: $0 PERFIL selftest|install|screenshot}
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

# kernel, initramfs y parámetros para arrancar la ISO directamente, con TEST como parámetro del autotest
live_args() {   # live_args TEST
	if [ "${BASE:-ubuntu}" = arch ]; then
		# ISO de archiso: el kernel y el initramfs se sacan de la propia ISO
		need_cmd xorriso
		xorriso -osirrox on -indev "$iso" -extract /arch/boot/x86_64/vmlinuz-linux "$out/vmlinuz" \
			-extract /arch/boot/x86_64/initramfs-linux.img "$out/initrd" >/dev/null 2>&1 ||
			die "no se pudo sacar el kernel de $iso"
		kernel=$out/vmlinuz initrd=$out/initrd
		label=$(xorriso -indev "$iso" -pvd_info 2>/dev/null | sed -n 's/^Volume Id *: *//p' | tr -d "'")
		append="archisobasedir=arch archisolabel=${label:-RROS_${profile^^}} lsm=landlock,lockdown,yama,integrity,apparmor,bpf"
		append+=" console=ttyS0 $1 loglevel=3"
	else
		kernel=$iso_dir/casper/vmlinuz initrd=$iso_dir/casper/initrd
		append="boot=casper quiet console=ttyS0 $1 loglevel=3"
	fi
}

# run_serial LOG MARCA ESPERA_APAGADO qemu-args...: arranca QEMU con la consola en LOG y espera a que aparezca
# MARCA_OK o MARCA_FAIL (o TIMEOUT); con ESPERA_APAGADO=1 deja después que el sistema se apague solo
# Con ASK y ANSWER, cuando ASK aparece en la consola se escribe ANSWER en ella (la contraseña del disco cifrado).
# Cuando el autotest escribe RROS_PANTALLA_LISTA (el escritorio ya está), se guarda la pantalla en SHOT (PNG).
run_serial() {
	local log=$1 mark=$2 wait_off=$3 waited=0 answered=0 shot_done=0 fifo=$out/consola.in sock=$out/monitor.sock
	shift 3
	: > "$log"
	rm -f "$fifo" "$sock"
	mkfifo "$fifo"
	exec 3<>"$fifo"
	qemu-system-x86_64 "${accel[@]}" -m "$ram" -smp 2 -nographic -no-reboot -monitor "unix:$sock,server,nowait" \
		"$@" < "$fifo" > "$log" 2>&1 &
	qpid=$!
	while kill -0 "$qpid" 2>/dev/null && [ "$waited" -lt "${TIMEOUT:-1500}" ]; do
		grep -aq "${mark}_\(OK\|FAIL\)" "$log" && break
		if [ -n "${SHOT:-}" ] && [ "$shot_done" -eq 0 ] && grep -aq RROS_PANTALLA_LISTA "$log"; then
			python3 "$ROOT_DIR/scripts/qemu-shot.py" "$sock" "$SHOT" || log "no se pudo capturar la pantalla"
			shot_done=1
		fi
		if [ -n "${ASK:-}" ] && [ "$answered" -eq 0 ] && grep -aq "$ASK" "$log"; then
			sleep 1
			printf '%s\r' "$ANSWER" >&3
			answered=1
			log "respondida la pregunta «$ASK»"
		fi
		sleep 2
		waited=$((waited + 2))
	done
	if [ "$wait_off" -eq 1 ]; then   # que termine de escribir en el disco antes de cortar
		waited=0
		while kill -0 "$qpid" 2>/dev/null && [ "$waited" -lt 180 ]; do
			sleep 2
			waited=$((waited + 2))
		done
	fi
	kill "$qpid" 2>/dev/null || true
	wait "$qpid" 2>/dev/null || true
	exec 3>&-
	rm -f "$fifo"
}

verdict() {   # verdict LOG MARCA QUÉ
	grep -a -E '^\[(ok|FALLO)\]|=== |^==> |^info: |^diag: ' "$1" | tr -d '\r' || true
	if grep -aq "${2}_OK" "$1"; then
		log "$3: CORRECTO"
	else
		# la consola sin el ruido de systemd al arrancar y apagar: quedan los errores y lo de Lily
		echo "--- consola serie (sin las líneas de estado de systemd) ---"
		tr -d '\r' < "$1" | grep -av -E '\[ *(OK|DEPEND) *\]|^ *(Starting|Stopping|Unmounting|Mounting) ' | tail -n 150
		die "$3: falló o no terminó (mira $1)"
	fi
}

case "$mode" in
	selftest)
		log "arranque por consola serie con autotest (RAM ${ram} MB); puede tardar varios minutos sin KVM"
		live_args rros.selftest
		SHOT=$out/escritorio.png run_serial "$out/serial.log" RROS_SELFTEST 0 -kernel "$kernel" -initrd "$initrd" \
			-append "$append" -cdrom "$iso"
		[ ! -s "$out/escritorio.png" ] || log "captura del escritorio: $out/escritorio.png"
		verdict "$out/serial.log" RROS_SELFTEST "autotest del live"
		;;
	install)
		# 1) el live instala en un disco virtual sin preguntas (rros.installtest); 2) se arranca desde el disco
		#    (GRUB del disco) y el sistema instalado pasa el autotest. CRYPT=1: cifrado con LUKS2 (la contraseña se
		#    escribe en la consola cuando el arranque la pide). FIRMWARE=uefi: se instala para UEFI y el disco
		#    arranca con OVMF (el live arranca igual, con el kernel de la ISO)
		need_cmd qemu-img
		disk=$out/disco.qcow2
		rm -f "$disk"
		qemu-img create -f qcow2 "$disk" "${DISK_GB:-16}G" >/dev/null
		test_arg=rros.installtest=/dev/vda
		[ "${CRYPT:-0}" = 1 ] && test_arg+=" rros.installtest.cifrado"
		fwargs=()
		case ${FIRMWARE:-bios} in
		bios) ;;
		uefi)
			test_arg+=" rros.installtest.uefi"
			cp /usr/share/OVMF/OVMF_VARS_4M.fd "$out/vars.fd"
			fwargs=(-drive "if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd"
				-drive "if=pflash,format=raw,file=$out/vars.fd") ;;
		*) die "FIRMWARE: bios o uefi" ;;
		esac
		log "instalación en un disco virtual de ${DISK_GB:-16} GB (RAM ${ram} MB, ${FIRMWARE:-bios})$([ "${CRYPT:-0}" = 1 ] && echo ', cifrada')"
		live_args "$test_arg"
		run_serial "$out/install.log" RROS_INSTALL 1 -kernel "$kernel" -initrd "$initrd" -append "$append" -cdrom "$iso" \
			-drive "file=$disk,if=virtio,format=qcow2"
		verdict "$out/install.log" RROS_INSTALL "instalación"
		log "arranque desde el disco instalado"
		if [ "${CRYPT:-0}" = 1 ]; then
			ASK="passphrase for" ANSWER=prueba-rros run_serial "$out/serial.log" RROS_SELFTEST 0 \
				"${fwargs[@]}" -drive "file=$disk,if=virtio,format=qcow2" -boot c
		else
			run_serial "$out/serial.log" RROS_SELFTEST 0 "${fwargs[@]}" -drive "file=$disk,if=virtio,format=qcow2" -boot c
		fi
		verdict "$out/serial.log" RROS_SELFTEST "autotest del sistema instalado"
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
