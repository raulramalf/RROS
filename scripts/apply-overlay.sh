#!/usr/bin/env bash
# Aplica la capa de RROS sobre un rootfs ya instalado: overlay + identidad.
# uso: apply-overlay.sh RAIZ PERFIL BASE   (BASE: arch)
# shellcheck source=common.sh
. "$(dirname "$0")/common.sh"

root=${1:?uso: $0 RAIZ PERFIL BASE}
profile=${2:?uso: $0 RAIZ PERFIL BASE}
base=${3:?uso: $0 RAIZ PERFIL BASE}
check_profile "$profile"
[ -d "$root" ] || die "no existe $root"

cp -a "$ROOT_DIR/overlay/common/." "$root/"
[ -d "$ROOT_DIR/overlay/$base" ] && cp -a "$ROOT_DIR/overlay/$base/." "$root/"
# Arch: aspecto de RROS para Plasma en todo el sistema (fondo, colores, barras de título, inicio de sesión)
[ "$base" = arch ] && "$ROOT_DIR/scripts/plasma-defaults.sh" "$root" "$profile"
"$ROOT_DIR/scripts/apply-identity.sh" "$root" "$profile" "$base"
mkdir -p "$root/etc/rros"
echo "$profile" > "$root/etc/rros/profile"
log "capa RROS aplicada ($base, $profile)"
