#!/usr/bin/env bash
# Genera la web estática de demostración (site/) que se puede publicar en Vercel, Netlify o GitHub Pages:
#   site/index.html        portada
#   site/escritorio.html   simulación del escritorio (se edita a mano)
#   site/ajustes.html      RROS Ajustes con un servidor simulado (de scripts/build-gui-demo.sh)
#   site/fondos/*.jpg      los fondos reales que genera rros-theme, por tema y estilo
# Requiere haber compilado (make) y python3 con Pillow para pasar los fondos a JPEG.
# shellcheck source=common.sh
. "$(dirname "$0")/common.sh"

[ -x "$RROS/rros-theme" ] || die "compila primero con 'make'"
need_cmd python3
out=$ROOT_DIR/site
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
mkdir -p "$out/fondos"

for th in aurora nord dracula gruvbox; do
	for st in aurora ondas degradado circulos; do
		rm -rf "$tmp/h" && mkdir -p "$tmp/h"
		HOME=$tmp/h XDG_CONFIG_HOME=$tmp/h/.config XDG_DATA_HOME=$tmp/h RROS_USER_CONF=$tmp/h/u.toml \
			"$RROS/rros-config" set ui.theme "$th" >/dev/null
		HOME=$tmp/h XDG_CONFIG_HOME=$tmp/h/.config XDG_DATA_HOME=$tmp/h RROS_USER_CONF=$tmp/h/u.toml \
			"$RROS/rros-theme" -p standard wallpaper --style "$st" --size 1280x720 --out "$tmp/$th-$st.png" >/dev/null
	done
done
python3 - "$tmp" "$out/fondos" <<'PY'
import glob, os, sys
from PIL import Image
src, dst = sys.argv[1], sys.argv[2]
for f in glob.glob(os.path.join(src, "*.png")):
    Image.open(f).convert("RGB").save(os.path.join(dst, os.path.basename(f)[:-4] + ".jpg"), quality=82)
PY

"$ROOT_DIR/scripts/build-gui-demo.sh" >/dev/null
{
	printf '<!doctype html>\n<html lang="es">\n<head>\n<meta charset="utf-8">\n'
	printf '<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">\n</head>\n<body>\n'
	cat "$ROOT_DIR/build/gui-demo.html"
	printf '\n</body>\n</html>\n'
} > "$out/ajustes.html"
log "web de demostración en $out (sírvela con: python3 -m http.server -d site)"
