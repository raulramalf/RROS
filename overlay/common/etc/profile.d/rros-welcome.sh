# Lily: ficha del sistema al abrir una sesión de login interactiva.
case $- in *i*) ;; *) return 0 ;; esac
[ -n "${RROS_WELCOMED:-}" ] && return 0
export RROS_WELCOMED=1
command -v rros-fetch >/dev/null 2>&1 && rros-fetch
