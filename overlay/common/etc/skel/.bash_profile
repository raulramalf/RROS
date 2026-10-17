# ~/.bash_profile - RROS
# ~/.local/bin en el PATH (scripts y extensiones `rros-NOMBRE` propios)
case ":$PATH:" in *":$HOME/.local/bin:"*) ;; *) PATH="$HOME/.local/bin:$PATH" ;; esac
export PATH
[ -f ~/.bashrc ] && . ~/.bashrc
# En tty1 sin sesión gráfica arranca el escritorio RROS directamente (sin gestor de pantalla: menos RAM).
# Solo en una consola de texto y si no hay gestor de inicio de sesión (con SDDM, Plasma también lee este archivo
# al arrancar, desde tty1). Para desactivarlo: touch ~/.rros-no-autostart
if [ -z "${WAYLAND_DISPLAY:-}${DISPLAY:-}" ] && [ "$(tty)" = /dev/tty1 ] \
   && [ "${XDG_SESSION_TYPE:-tty}" = tty ] && [ ! -e /etc/systemd/system/display-manager.service ] \
   && [ ! -e ~/.rros-no-autostart ] && command -v rros-session >/dev/null 2>&1; then
	exec rros-session
fi
