#!/usr/bin/env bash
# Iconos de las apps del catálogo de la Tienda, guardados en la imagen (/usr/share/rros/tienda/iconos/ID.png) para que
# se vean sin internet: del AppStream de Arch (archlinux-appstream-data, si está en el equipo que compila) y, si no, de
# Flathub. Nunca hace fallar la compilación: lo que no encuentra se queda con la ficha de la inicial.
#   store-icons.sh CATALOGO.toml DESTINO
set -u
cat=${1:?catálogo}; out=${2:?destino}
mkdir -p "$out"
sw=/usr/share/swcatalog
# AppStream de Arch: en el XML de cada repositorio, el paquete (<pkgname>) y su icono guardado (<icon type="cached">);
# el archivo está en icons/CARPETA/TAMAÑO/. Sale una línea «paquete|ruta del icono» por paquete.
map=$(mktemp)
if [ -d "$sw/xml" ] && command -v python3 >/dev/null; then
	python3 - "$sw" > "$map" <<'PY' || true
import glob, gzip, os, sys
import xml.etree.ElementTree as ET
sw = sys.argv[1]
# los iconos pueden estar en cualquier carpeta de icons/ (una por repositorio): se buscan por nombre
dirs = sorted(glob.glob(f"{sw}/icons/*/"))
for x in sorted(glob.glob(f"{sw}/xml/*.xml.gz")):
    for c in ET.parse(gzip.open(x)).getroot().iter("component"):
        pkg = c.findtext("pkgname")
        if not pkg:
            continue
        best = None
        for ic in c.findall("icon"):
            if ic.get("type") != "cached" or not ic.text:
                continue
            for size in ("128x128", "64x64"):
                for d in dirs:
                    f = f"{d}{size}/{ic.text.strip()}"
                    if os.path.isfile(f) and (best is None or best[0] < int(size.split("x")[0])):
                        best = (int(size.split("x")[0]), f)
        if best:
            print(f"{pkg}|{best[1]}")
PY
fi
n=0 miss=0
while IFS='|' read -r id pkg fp cmd; do
	f="$out/$id.png"
	[ -s "$f" ] && continue
	src=""
	for name in "$pkg" "$cmd"; do
		[ -n "$name" ] || continue
		src=$(awk -F'|' -v p="$name" '$1 == p { print $2; exit }' "$map")
		[ -n "$src" ] && break
	done
	if [ -n "$src" ]; then
		cp "$src" "$f"
	elif [ -n "$fp" ]; then
		curl -fsL --max-time 20 -o "$f" "https://dl.flathub.org/repo/appstream/x86_64/icons/128x128/$fp.png" || rm -f "$f"
	fi
	if [ -s "$f" ]; then n=$((n + 1)); else miss=$((miss + 1)); fi
done < <(awk '
	/^\[app\./ { if (id != "") print id "|" pkg "|" fp "|" cmd; id = substr($0, 6, length($0) - 6); pkg = fp = cmd = "" }
	/^pacman *=/ { v = $0; sub(/^[^"]*"/, "", v); sub(/".*/, "", v); split(v, a, ","); pkg = a[1] }
	/^flatpak *=/ { v = $0; sub(/^[^"]*"/, "", v); sub(/".*/, "", v); fp = v }
	/^cmd *=/ { v = $0; sub(/^[^"]*"/, "", v); sub(/".*/, "", v); cmd = v }
	END { if (id != "") print id "|" pkg "|" fp "|" cmd }' "$cat")
rm -f "$map"
echo "store-icons: $n iconos guardados, $miss sin icono (se verá la inicial)"
exit 0
