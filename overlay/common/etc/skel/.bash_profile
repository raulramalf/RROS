# ~/.bash_profile - RROS
[ -f ~/.bashrc ] && . ~/.bashrc
# En tty1 sin sesión gráfica arranca el escritorio RROS directamente (sin gestor de pantalla: menos RAM).
# Para desactivarlo: touch ~/.rros-no-autostart
if [ -z "${WAYLAND_DISPLAY:-}${DISPLAY:-}" ] && [ "$(tty)" = /dev/tty1 ] \
   && [ ! -e ~/.rros-no-autostart ] && command -v rros-session >/dev/null 2>&1; then
	exec rros-session
fi
