#!/usr/bin/env bash
# Genera build/gui-demo.html: la GUI de ajustes con un servidor simulado, para verla sin instalar nada.
# El estado inicial (claves, temas, paletas) sale de `rros-settings state` con una configuración limpia.
# shellcheck source=common.sh
. "$(dirname "$0")/common.sh"

[ -x "$RROS/rros-settings" ] || die "compila primero con 'make'"
out=$RROS/gui-demo.html
tmp=$(mktemp -d)
trap 'rm -rf "${tmp:?}"' EXIT

RROS_PROFILE_DIR="$ROOT_DIR/profiles" RROS_PROFILE_FILE="$tmp/profile" RROS_USER_CONF="$tmp/user.toml" \
	RROS_SYSTEM_CONF="$tmp/sys.toml" RROS_THEMES_DIR="$tmp/themes" XDG_CONFIG_HOME="$tmp/xdg" \
	"$RROS/rros-settings" state > "$tmp/state.json"

# catálogo de aplicaciones: sin nada instalado (dpkg-query y flatpak simulados), perfil standard
mkdir -p "$tmp/bin"
printf '#!/bin/sh\nexit 1\n' > "$tmp/bin/dpkg-query"
printf '#!/bin/sh\nexit 1\n' > "$tmp/bin/flatpak"
chmod +x "$tmp/bin/dpkg-query" "$tmp/bin/flatpak"
PATH="$tmp/bin:$PATH" RROS_PROFILE_DIR="$ROOT_DIR/profiles" RROS_PROFILE_FILE="$tmp/profile" \
	RROS_APPS_CATALOG="$ROOT_DIR/apps/catalog.toml" "$RROS/rros-apps" list --json > "$tmp/apps.json"

# awk -v interpretaría las barras invertidas del JSON: se lee desde archivo
awk -v statefile="$tmp/state.json" -v appsfile="$tmp/apps.json" -v mock="$ROOT_DIR/gui/mock.js" '
	!done && /^<script>$/ {
		print "<script>\nwindow.RROS_MOCK_STATE ="
		while ((getline line < statefile) > 0) print line
		print ";\n</script>"
		print "<script>\nwindow.RROS_MOCK_APPS ="
		while ((getline line < appsfile) > 0) print line
		print ";\n</script>"
		print "<script>"
		while ((getline line < mock) > 0) print line
		print "</script>"
		done = 1
	}
	{ print }
' "$ROOT_DIR/gui/index.html" > "$out"
log "demo generada: $out ($(wc -c < "$out") bytes)"
