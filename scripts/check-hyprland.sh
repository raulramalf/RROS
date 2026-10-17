#!/usr/bin/env bash
# Valida con el propio Hyprland (Hyprland --verify-config) la configuración que genera rros-theme para
# cada perfil, estilo de escritorio y modo de ventanas, también con atajos y reglas del usuario.
# Además prueba una a una las formas de reglas y atajos que salen de las traducciones.
# Se ejecuta en Arch (en CI) como usuario normal, tras `make`.
# shellcheck source=common.sh
. "$(dirname "$0")/common.sh"

need_cmd Hyprland
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
export RROS_PROFILE_DIR=$ROOT_DIR/profiles RROS_SYSTEM_CONF=$T/sys.toml XDG_RUNTIME_DIR=$T/run HOME=$T/home
mkdir -p "$T/run" "$T/home"
chmod 700 "$T/run"
log "Hyprland $(Hyprland --version 2>/dev/null | head -1)"

bad=0
verify() {   # verify NOMBRE ARCHIVO
	local outf=$T/verify.txt
	if Hyprland --verify-config -c "$2" > "$outf" 2>&1 && ! grep -qiE 'error|invalid' "$outf"; then
		echo "ok      $1"
	else
		echo "FALLO   $1"
		sed 's/^/        /' "$outf" | head -30
		bad=$((bad + 1))
	fi
}

n=0
for profile in lite standard pro; do
	for desk in clasico moderno mosaico; do
		for layout in floating tiling hybrid; do
			n=$((n + 1))
			export RROS_USER_CONF=$T/u$n.toml
			"$RROS/rros-theme" -p "$profile" escritorio "$desk" >/dev/null
			"$RROS/rros-config" set ui.layout "$layout" >/dev/null
			"$RROS/rros-theme" -p "$profile" render --out "$T/o$n" >/dev/null
			verify "$profile $desk $layout" "$T/o$n/hypr/hyprland.conf"
		done
	done
done
# lo del usuario: atajos, reglas e inicio, y todas las opciones de efectos
export RROS_USER_CONF=$T/usuario.toml
cat > "$RROS_USER_CONF" <<'EOF'
[ui]
effects = "advanced"
blur = true
shadows = true
dim_inactive = true
animation_style = "rebote"
tiling_layout = "master"
dock = true
[keybind]
web = "Mod+Shift+b exec firefox"
mv = "Mod+F2 move container to workspace number 3"
foco = "Mod+o focus left"
flota = "Mod+Shift+f floating toggle"
pestanas = "Mod+Shift+g layout tabbed"
[rule]
calc = "app_id=galculator floating"
mpv = "app_id=mpv workspace 3"
notas = "title=Notas opacity 0.8"
sinborde = "class=Steam border none"
[autostart]
nube = "nextcloud --background"
EOF
"$RROS/rros-theme" -p pro render --out "$T/usr" >/dev/null
verify "usuario (atajos, reglas, efectos)" "$T/usr/hypr/hyprland.conf"

# cada forma que escriben las traducciones de atajos y reglas, una a una
probe() {   # probe LÍNEA...
	local f=$T/probe.conf
	printf '%s\n' "$@" > "$f"
	if Hyprland --verify-config -c "$f" > "$T/probe.txt" 2>&1 && ! grep -qiE 'error|invalid' "$T/probe.txt"; then
		echo "ok      $*"
	else
		echo "FALLO   $*: $(grep -iE 'error|invalid' "$T/probe.txt" | tail -1)"
		bad=$((bad + 1))
	fi
}
log "reglas y atajos traducidos"
m="match:class ^(prueba)\$"
for eff in "float on" "pin on" "fullscreen on" "workspace 3" "opacity 0.8" "idle_inhibit fullscreen" \
	"border_size 0" "border_size 2" "group set"; do
	probe "windowrule = $eff, $m"
done
probe "windowrule = float on, match:title ^(Notas)\$"
for d in "layoutmsg, togglesplit" "pin," "movetoworkspacesilent, special" "togglespecialworkspace," \
	"togglegroup," "changegroupactive, f" "movefocus, l" "movewindow, r" "fullscreen, 0" "togglefloating,"; do
	probe "bind = SUPER, X, $d"
done

[ "$bad" -eq 0 ] || die "$bad configuraciones de Hyprland no válidas"
log "configuraciones de Hyprland válidas: $((n + 1)), y todas las formas traducidas"
