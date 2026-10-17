#!/usr/bin/env bash
# Aplica la capa de RROS sobre un rootfs ya instalado: overlay + identidad.
# uso: apply-overlay.sh RAIZ PERFIL BASE   (BASE: ubuntu | arch)
# shellcheck source=common.sh
. "$(dirname "$0")/common.sh"

root=${1:?uso: $0 RAIZ PERFIL BASE}
profile=${2:?uso: $0 RAIZ PERFIL BASE}
base=${3:?uso: $0 RAIZ PERFIL BASE}
check_profile "$profile"
[ -d "$root" ] || die "no existe $root"

cp -a "$ROOT_DIR/overlay/common/." "$root/"
[ -d "$ROOT_DIR/overlay/$base" ] && cp -a "$ROOT_DIR/overlay/$base/." "$root/"
# Lo propio de Ubuntu que no debe verse en RROS: avisos legales de inicio de sesión (documentación,
# Landscape, Ubuntu Pro...) y los ajustes de casper para escritorios de Ubuntu que aquí no existen.
if [ "$base" = ubuntu ]; then
	rm -f "${root:?}"/etc/update-motd.d/*
	rm -rf "${root:?}/etc/legal"
	for hook in 22desktop_settings 31disable_update_notifier 33enable_apport_crashes 34disable_kde_services \
	            37disable_screensaver_lubuntu 45jackd2 52gnome_initial_setup 53disable_unattended_upgrades \
	            55disable_snap_refresh 57pollinate 59disable_mozc_autosetup 61desktop_canary_tweaks; do
		rm -f "${root:?}/usr/share/initramfs-tools/scripts/casper-bottom/$hook"
	done
fi
# Arch: aspecto de RROS para Plasma en todo el sistema (fondo, colores, barras de título, inicio de sesión)
[ "$base" = arch ] && "$ROOT_DIR/scripts/plasma-defaults.sh" "$root" "$profile"
"$ROOT_DIR/scripts/apply-identity.sh" "$root" "$profile" "$base"
mkdir -p "$root/etc/rros"
echo "$profile" > "$root/etc/rros/profile"
log "capa RROS aplicada ($base, $profile)"
