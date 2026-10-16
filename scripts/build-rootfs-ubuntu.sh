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
trap 'umount -lf "$out/dev" "$out/proc" "$out/sys" 2>/dev/null || true' EXIT

cat > "$out/etc/apt/sources.list" <<APT
deb $mirror $suite main universe
deb $mirror $suite-updates main universe
deb http://security.ubuntu.com/ubuntu $suite-security main universe
APT
cat > "$out/etc/apt/apt.conf.d/99rros" <<'APT'
APT::Install-Recommends "false";
APT::Install-Suggests "false";
APT

pkgs=$(read_list "$ROOT_DIR/packages/ubuntu-base.list" "$ROOT_DIR/packages/ubuntu-$profile.list")
log "instalando paquetes: $pkgs"
chroot "$out" apt-get update
# shellcheck disable=SC2086
DEBIAN_FRONTEND=noninteractive chroot "$out" apt-get install -y $pkgs

log "instalando componentes RROS"
make -C "$ROOT_DIR" install DESTDIR="$out" PREFIX=/usr
echo "$profile" > "$out/etc/rros/profile"
echo "rros-$profile" > "$out/etc/hostname"
sed -i -e 's/^NAME=.*/NAME="RROS"/' -e 's/^PRETTY_NAME=.*/PRETTY_NAME="RROS ('"$profile"')"/' \
	"$out/etc/os-release" 2>/dev/null || true
chroot "$out" systemctl enable rros-firstboot.service || true

log "limpieza"
chroot "$out" apt-get clean
rm -rf "${out:?}"/var/lib/apt/lists/* "${out:?}"/var/cache/apt/* "${out:?}"/tmp/* \
       "${out:?}"/usr/share/doc/* "${out:?}"/usr/share/man/* "${out:?}"/usr/share/info/*
find "${out:?}/usr/share/locale" -mindepth 1 -maxdepth 1 ! -name 'es*' ! -name 'en*' ! -name locale.alias \
	-exec rm -rf {} + 2>/dev/null || true

log "presupuesto de disco"
"$RROS/rros-budget" disk "$out" -p "$profile"
