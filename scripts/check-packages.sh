#!/usr/bin/env bash
# Comprueba que todos los paquetes de packages/ubuntu-*.list existen en el archivo de Ubuntu.
# Requiere `apt-get update` hecho en un Ubuntu 24.04 (noble).
# shellcheck source=common.sh
. "$(dirname "$0")/common.sh"

need_cmd apt-cache
bad=0 total=0
for list in "$ROOT_DIR"/packages/ubuntu-*.list; do
	for pkg in $(read_list "$list"); do
		total=$((total + 1))
		cand=$(apt-cache policy "$pkg" 2>/dev/null | sed -n 's/^  Candidate: //p')
		if [ -z "$cand" ] || [ "$cand" = "(none)" ]; then
			echo "FALTA: $pkg  ($(basename "$list"))" >&2
			bad=$((bad + 1))
		fi
	done
done
echo "$total paquetes comprobados, $bad que faltan"
[ "$bad" -eq 0 ]
