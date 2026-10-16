#!/usr/bin/env bash
# shellcheck disable=SC2034
# Funciones comunes de los scripts de build de RROS.
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
RROS=$ROOT_DIR/build
# las herramientas que se ejecutan en el anfitrión (rros-budget...) leen los perfiles del repositorio
export RROS_PROFILE_DIR=${RROS_PROFILE_DIR:-$ROOT_DIR/profiles}

die() { echo "error: $*" >&2; exit 1; }
log() { echo "==> $*"; }

need_root() { [ "$(id -u)" -eq 0 ] || die "ejecuta como root (sudo)"; }
need_cmd() { for c in "$@"; do command -v "$c" >/dev/null || die "falta el comando '$c'"; done; }

check_profile() {
	case "$1" in lite | standard | pro) ;; *) die "perfil inválido: '$1' (lite|standard|pro)" ;; esac
}

# Lee una clave de un archivo de perfil con rros-config.
profile_get() { "$RROS/rros-config" -f "$ROOT_DIR/profiles/$1.toml" get "$2"; }

# Lista de paquetes sin comentarios ni líneas vacías.
read_list() { sed -e 's/#.*//' -e '/^[[:space:]]*$/d' "$@" | tr '\n' ' '; }

# Ejecuta un comando dentro de un rootfs, con /dev, /proc, /sys y /dev/pts montados.
# uso: chroot_run RAIZ comando...
chroot_run() {
	local root=$1 rc=0
	shift
	[ -d "$root" ] || die "no existe $root"
	mount --bind /dev "$root/dev"
	mount -t proc proc "$root/proc"
	mount -t sysfs sys "$root/sys"
	mount -t devpts devpts "$root/dev/pts" 2>/dev/null || true
	chroot "$root" "$@" || rc=$?
	umount -lf "$root/dev/pts" "$root/dev" "$root/proc" "$root/sys" 2>/dev/null || true
	return "$rc"
}
