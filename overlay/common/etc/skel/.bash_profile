# ~/.bash_profile - RROS
# ~/.local/bin en el PATH (scripts y extensiones `rros-NOMBRE` propios)
case ":$PATH:" in *":$HOME/.local/bin:"*) ;; *) PATH="$HOME/.local/bin:$PATH" ;; esac
export PATH
[ -f ~/.bashrc ] && . ~/.bashrc
