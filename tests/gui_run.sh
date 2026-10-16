#!/usr/bin/env bash
# Arranca rros-settings contra una configuración temporal y ejecuta las pruebas de la GUI con un navegador.
# Necesita Playwright: PWPATH=<ruta a playwright> tests/gui_run.sh
set -euo pipefail
cd "$(dirname "$0")/.."
: "${PWPATH:?define PWPATH con la ruta a playwright}"
T=$(mktemp -d)
trap 'curl -s -H "X-RROS-Token: tk" -X POST "http://127.0.0.1:18769/api/quit" >/dev/null 2>&1 || true; rm -rf "${T:?}"' EXIT
mkdir -p "$T/bin"
# terminal falsa: anota con qué argumentos la abre la tienda
printf '#!/bin/sh\nprintf "%%s\\n" "$@" > "%s/foot-args.txt"\n' "$T" > "$T/bin/foot"
chmod +x "$T/bin/foot"
export RROS_PROFILE_DIR=$PWD/profiles RROS_PROFILE_FILE=$T/profile RROS_USER_CONF=$T/u.toml \
	RROS_SYSTEM_CONF=$T/s.toml XDG_CONFIG_HOME=$T/xdg RROS_THEMES_DIR=$T/themes RROS_GUI_DIR=$PWD/gui \
	RROS_APPS_CATALOG=$PWD/apps/catalog.toml PATH="$T/bin:$PATH"
build/rros-settings web --no-open --port 18769 --token tk --timeout 300 > "$T/server.log" 2>&1 &
sleep 0.8
RROS_TEST_DIR=$T PORT=18769 TOKEN=tk node tests/gui_test.js
PWPATH=$PWPATH node tests/gui_demo_test.js
PWPATH=$PWPATH node tests/preview_test.js
