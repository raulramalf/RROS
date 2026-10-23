#!/usr/bin/env bash
# Iconos de las apps del catálogo de la Tienda, guardados en la imagen (/usr/share/rros/tienda/iconos/ID.png) para que
# se vean sin internet: del AppStream de Arch (archlinux-appstream-data, si está en el equipo que compila) y, si no, de
# Flathub. Nunca hace fallar la compilación: lo que no encuentra se queda con la ficha de la inicial.
#   store-icons.sh CATALOGO.toml DESTINO
set -u
cat=${1:?catálogo}; out=${2:?destino}
mkdir -p "$out"
sw=/usr/share/swcatalog/icons
n=0 miss=0
while IFS='|' read -r id pkg fp cmd; do
	f="$out/$id.png"
	[ -s "$f" ] && continue
	src=""
	for name in "$pkg" "$cmd"; do
		if [ -z "$name" ] || [ ! -d "$sw" ]; then continue; fi
		src=$(find "$sw" -path '*/128x128/*' \( -name "${name}_*.png" -o -name "${name}.png" \) 2>/dev/null | head -1)
		[ -n "$src" ] || src=$(find "$sw" -path '*/64x64/*' \( -name "${name}_*.png" -o -name "${name}.png" \) 2>/dev/null | head -1)
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
echo "store-icons: $n iconos guardados, $miss sin icono (se verá la inicial)"
exit 0
