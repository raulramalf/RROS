#!/usr/bin/env bash
# Comprueba que todos los paquetes de las listas de Arch y los del catálogo de la tienda (campo pacman)
# existen en los repositorios (pacman -Si).
# Se ejecuta en un sistema Arch (en CI, el contenedor archlinux) con la base de datos sincronizada.
# shellcheck source=common.sh
. "$(dirname "$0")/common.sh"

need_cmd pacman
missing=0 total=0
for list in "$ROOT_DIR"/packages/arch-*.list; do
	for p in $(read_list "$list"); do
		total=$((total + 1))
		if ! pacman -Si "$p" >/dev/null 2>&1 && ! pacman -Sg "$p" >/dev/null 2>&1; then
			echo "falta: $p ($(basename "$list"))"
			missing=$((missing + 1))
		fi
	done
done
# tienda: pacman = "a,b" en apps/catalog.toml
while IFS=, read -r -a pkgs; do
	for p in "${pkgs[@]}"; do
		total=$((total + 1))
		if ! pacman -Si "$p" >/dev/null 2>&1; then
			echo "falta: $p (apps/catalog.toml)"
			missing=$((missing + 1))
		fi
	done
done < <(sed -n 's/^pacman *= *"\(.*\)"/\1/p' "$ROOT_DIR/apps/catalog.toml")
echo "$total paquetes comprobados, $missing que faltan"
[ "$missing" -eq 0 ]
