#!/usr/bin/env bash
# Aspecto de RROS para Plasma en todo el sistema (sobre una raíz ya preparada): fondo, esquema de colores, barras
# de título, la barra y el dock de la primera vez y el fondo del menú de arranque, generados por rros-theme con el
# tema Lirio oscuro (los colores del logotipo).
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
	"$root/usr/share/grub/themes/rros"
run "$RROS/rros-theme" -p "$profile" wallpaper --style aurora --size 1920x1080 --out "$wp/contents/images/1920x1080.png" >/dev/null
cat > "$wp/metadata.json" <<'JSON'
{ "KPlugin": { "Id": "RROS", "Name": "RROS", "Authors": [ { "Name": "RROS" } ], "License": "CC-BY-SA-4.0" } }
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
# el menú de arranque (GRUB) con el mismo fondo
cp "$wp/contents/images/1920x1080.png" "$root/usr/share/grub/themes/rros/background.png"
# la barra y el dock de la primera vez: el mismo guion que genera rros-theme para el perfil, más el fondo. Su copia
# en ~/.local/state/rros le dice a rros-theme que ya está puesto (así el primer inicio no rehace los paneles)
run "$RROS/rros-theme" -p "$profile" render --out "$tmp/render" >/dev/null
lnf=$root/usr/share/plasma/look-and-feel/org.rros.desktop/contents/layouts
mkdir -p "$lnf" "$root/etc/skel/.local/state/rros"
cp "$tmp/render/plasma/layout.js" "$root/etc/skel/.local/state/rros/plasma-layout.js"
{
	cat "$tmp/render/plasma/layout.js"
	cat <<'JS'

// --- fondo de RROS (rros-theme pone después el del tema de cada usuario) ---
desktops().forEach(function (d) {
    d.wallpaperPlugin = "org.kde.image";
    d.currentConfigGroup = ["Wallpaper", "org.kde.image", "General"];
    d.writeConfig("Image", "file:///usr/share/wallpapers/RROS/contents/images/1920x1080.png");
});
JS
} > "$lnf/org.kde.plasma.desktop-layout.js"
log "aspecto de Plasma de RROS escrito en $root"
