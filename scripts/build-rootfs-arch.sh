#!/usr/bin/env bash
# Construye el rootfs de RROS sobre Arch Linux (pacstrap). Requiere root y arch-install-scripts.
# uso: build-rootfs-arch.sh PERFIL
# shellcheck source=common.sh
. "$(dirname "$0")/common.sh"

profile=${1:?uso: $0 PERFIL}
out=$ROOT_DIR/build/rootfs-arch-$profile

check_profile "$profile"
need_root
need_cmd pacstrap arch-chroot
[ -x "$RROS/rros-config" ] || die "compila primero con 'make'"

pkgs=$(read_list "$ROOT_DIR/packages/arch-base.list" "$ROOT_DIR/packages/arch-$profile.list")
log "pacstrap -> $out"
rm -rf "${out:?}"
mkdir -p "$out"
# shellcheck disable=SC2086
pacstrap -c -K "$out" $pkgs

make -C "$ROOT_DIR" install DESTDIR="$out" PREFIX=/usr
echo "$profile" > "$out/etc/rros/profile"
echo "rros-$profile" > "$out/etc/hostname"
arch-chroot "$out" systemctl enable rros-firstboot.service || true

rm -rf "${out:?}"/var/cache/pacman/pkg/* "${out:?}"/usr/share/doc/* "${out:?}"/usr/share/man/* \
       "${out:?}"/usr/share/info/*
"$RROS/rros-budget" disk "$out" -p "$profile"
