#!/usr/bin/env bash
# shellcheck disable=SC2034
# Funciones comunes de los scripts de build de RROS.
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
RROS=$ROOT_DIR/build

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
