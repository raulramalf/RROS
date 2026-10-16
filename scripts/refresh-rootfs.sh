#!/usr/bin/env bash
# Reinstala los componentes de RROS y su capa de identidad en un rootfs ya construido.
# Sirve para iterar sin repetir la descarga de paquetes. Requiere root.
# uso: refresh-rootfs.sh PERFIL
# shellcheck source=common.sh
. "$(dirname "$0")/common.sh"

profile=${1:?uso: $0 PERFIL}
check_profile "$profile"
need_root
out=$ROOT_DIR/build/rootfs-ubuntu-$profile
[ -d "$out" ] || die "no existe $out (ejecuta build-rootfs-ubuntu.sh)"

make -C "$ROOT_DIR" install DESTDIR="$out" PREFIX=/usr
"$ROOT_DIR/scripts/apply-overlay.sh" "$out" "$profile" ubuntu
# el usuario live lo crea casper al arrancar: no debe haber uno precreado ni restos de pruebas
if grep -q '^rros:' "$out/etc/passwd"; then
	chroot_run "$out" userdel -r rros 2>/dev/null || true
fi
log "regenerando el initramfs"
chroot_run "$out" update-initramfs -u -k all
systemctl --root="$out" enable rros-firstboot.service rros-apply.service rros-live.service rros-selftest.service rros-serial-shell.service >/dev/null 2>&1 || true
systemctl --root="$out" mask casper-md5check.service >/dev/null 2>&1 || true
log "rootfs actualizado"
"$RROS/rros-budget" disk "$out" -p "$profile"
