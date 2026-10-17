#!/usr/bin/env bash
# Construye el rootfs de RROS sobre Ubuntu (debootstrap). Requiere root.
# uso: build-rootfs-ubuntu.sh PERFIL [ARQ]   (ARQ: amd64 | arm64)
# shellcheck source=common.sh
. "$(dirname "$0")/common.sh"

profile=${1:?uso: $0 PERFIL [ARQ]}
arch=${2:-amd64}
suite=${SUITE:-noble}
mirror=${MIRROR:-http://archive.ubuntu.com/ubuntu}
out=$ROOT_DIR/build/rootfs-ubuntu-$profile

check_profile "$profile"
need_root
need_cmd debootstrap chroot
[ -x "$RROS/rros-config" ] || die "compila primero con 'make'"

log "debootstrap $suite ($arch) -> $out"
rm -rf "${out:?}"
debootstrap --variant=minbase --arch "$arch" "$suite" "$out" "$mirror"

mount --bind /dev "$out/dev"
mount -t proc proc "$out/proc"
mount -t sysfs sys "$out/sys"
mount -t devpts devpts "$out/dev/pts" 2>/dev/null || true
trap 'umount -lf "$out/dev/pts" "$out/dev" "$out/proc" "$out/sys" 2>/dev/null || true' EXIT

cat > "$out/etc/apt/sources.list" <<APT
deb $mirror $suite main universe
deb $mirror $suite-updates main universe
deb http://security.ubuntu.com/ubuntu $suite-security main universe
APT
cat > "$out/etc/apt/apt.conf.d/99rros" <<'APT'
APT::Install-Recommends "false";
APT::Install-Suggests "false";
APT

pkgs=$(read_list "$ROOT_DIR/packages/ubuntu-base.list" "$ROOT_DIR/packages/ubuntu-$profile.list" "$ROOT_DIR/packages/ubuntu-live.list")
log "instalando paquetes: $pkgs"
chroot "$out" apt-get update
# shellcheck disable=SC2086
DEBIAN_FRONTEND=noninteractive chroot "$out" apt-get install -y $pkgs
# idiomas del sistema (el de por defecto lo fija /etc/default/locale y locale.language)
chroot "$out" locale-gen es_ES.UTF-8 en_US.UTF-8

log "instalando componentes RROS"
make -C "$ROOT_DIR" install DESTDIR="$out" PREFIX=/usr
"$ROOT_DIR/scripts/apply-overlay.sh" "$out" "$profile" ubuntu
# mapa de teclado de la consola ya generado (español): lo cargan keyboard-setup y el initramfs
chroot "$out" /usr/libexec/rros-vconsole --save
# casper lee su configuración (usuario y nombre de equipo) desde el initramfs: se regenera tras la capa RROS
log "regenerando el initramfs"
chroot "$out" update-initramfs -u -k all
chroot "$out" systemctl enable rros-firstboot.service rros-apply.service rros-live.service rros-selftest.service rros-serial-shell.service rros-vconsole.service rros-live-userns.service || true
chroot "$out" systemctl mask casper-md5check.service || true
# blueman-mechanism se activa por D-Bus cuando hace falta; arrancarlo en el boot solo añade RAM y, en máquinas lentas, un fallo
chroot "$out" systemctl disable blueman-mechanism.service || true
# cortafuegos desde el primer arranque (rros-apply genera /etc/nftables.conf)
chroot "$out" systemctl enable nftables.service || true
chroot "$out" systemctl mask snapd.service snapd.socket apport.service whoopsie.service motd-news.timer ua-timer.timer 2>/dev/null || true

log "limpieza"
chroot "$out" apt-get clean
rm -rf "${out:?}"/var/lib/apt/lists/* "${out:?}"/var/cache/apt/* "${out:?}"/tmp/* \
       "${out:?}"/usr/share/doc/* "${out:?}"/usr/share/man/* "${out:?}"/usr/share/info/*
find "${out:?}/usr/share/locale" -mindepth 1 -maxdepth 1 ! -name 'es*' ! -name 'en*' ! -name locale.alias \
	-exec rm -rf {} + 2>/dev/null || true

log "presupuesto de disco"
"$RROS/rros-budget" disk "$out" -p "$profile"
