#!/usr/bin/env bash
# Aspecto de RROS para Plasma en todo el sistema (sobre una raíz ya preparada): fondo, esquema de colores, barras
# de título y fondo de la pantalla de inicio de sesión, generados por rros-theme con el tema Aurora oscuro.
# Así el primer arranque ya se ve como RROS; al iniciar sesión, rros-theme lo rehace con el tema de cada usuario.
# uso: plasma-defaults.sh RAIZ PERFIL
# shellcheck source=common.sh
. "$(dirname "$0")/common.sh"

root=${1:?uso: $0 RAIZ PERFIL}
profile=${2:?uso: $0 RAIZ PERFIL}
[ -x "$RROS/rros-theme" ] || die "compila primero con 'make'"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
run() { HOME=$tmp XDG_CONFIG_HOME=$tmp/config XDG_DATA_HOME=$tmp/data RROS_USER_CONF=$tmp/u.toml RROS_PLASMA_TEST=1 "$@"; }

wp=$root/usr/share/wallpapers/RROS
mkdir -p "$wp/contents/images" "$root/usr/share/color-schemes" "$root/usr/share/aurorae/themes" \
	"$root/usr/share/sddm/themes/breeze"
run "$RROS/rros-theme" -p "$profile" wallpaper --style aurora --size 1920x1080 --out "$wp/contents/images/1920x1080.png" >/dev/null
cat > "$wp/metadata.json" <<'JSON'
{ "KPlugin": { "Id": "RROS", "Name": "RROS Aurora", "Authors": [ { "Name": "RROS" } ], "License": "CC-BY-SA-4.0" } }
JSON
run "$RROS/rros-theme" -p "$profile" apply --quiet
cp "$tmp"/data/color-schemes/RROS-*.colors "$root/usr/share/color-schemes/RROS.colors"
rm -rf "$root/usr/share/aurorae/themes/RROS"
cp -r "$tmp/data/aurorae/themes/RROS" "$root/usr/share/aurorae/themes/RROS"
# kdeglobals: los valores de RROS más los colores (sin el grupo [General] del esquema, que ya está)
if [ -f "$root/etc/xdg/kdeglobals.in" ]; then
	{
		cat "$root/etc/xdg/kdeglobals.in"
		echo
		awk '/^\[/ { keep = ($0 !~ /^\[General\]/) } keep' "$root/usr/share/color-schemes/RROS.colors"
	} > "$root/etc/xdg/kdeglobals"
	rm -f "$root/etc/xdg/kdeglobals.in"
fi
# pantalla de inicio de sesión (tema Breeze de SDDM) con el mismo fondo
printf '[General]\nbackground=/usr/share/wallpapers/RROS/contents/images/1920x1080.png\ntype=image\n' \
	> "$root/usr/share/sddm/themes/breeze/theme.conf.user"
log "aspecto de Plasma de RROS escrito en $root"
