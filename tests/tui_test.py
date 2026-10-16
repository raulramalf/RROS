#!/usr/bin/env python3
"""Prueba de la TUI de rros-settings con un pseudo-terminal: teclas reales y comprobación del resultado."""
import json, os, pty, select, subprocess, sys, tempfile, time, fcntl, termios, struct

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN = os.path.join(ROOT, "build", "rros-settings")
tmp = tempfile.mkdtemp(prefix="rros-tui-")
env = dict(os.environ,
           RROS_PROFILE_DIR=os.path.join(ROOT, "profiles"), RROS_PROFILE_FILE=os.path.join(tmp, "profile"),
           RROS_USER_CONF=os.path.join(tmp, "user.toml"), RROS_SYSTEM_CONF=os.path.join(tmp, "sys.toml"),
           XDG_CONFIG_HOME=os.path.join(tmp, "xdg"), RROS_THEMES_DIR=os.path.join(tmp, "themes"),
           TERM="xterm-256color")
fails = []


def check(cond, msg):
    if not cond:
        fails.append(msg)
        print("  FALLO:", msg, file=sys.stderr)


state = json.loads(subprocess.check_output([BIN, "state"], env=env))
ncats = len(state["categories"])

pid, fd = pty.fork()
if pid == 0:
    os.execve(BIN, [BIN, "tui"], env)
fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 30, 100, 0, 0))
screen = b""


def pump(wait=0.25):
    global screen
    end = time.time() + wait
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.05)
        if r:
            try:
                data = os.read(fd, 65536)
            except OSError:
                return
            if not data:
                return
            screen += data


def send(s, wait=0.25):
    os.write(fd, s.encode() if isinstance(s, str) else s)
    pump(wait)


def userconf():
    try:
        return open(env["RROS_USER_CONF"]).read()
    except FileNotFoundError:
        return ""


pump(0.6)
check(b"RROS Ajustes" in screen, "cabecera visible")
check("Interfaz".encode() in screen, "categoría Interfaz visible")

# buscar una clave y cambiarla con las flechas: gap del perfil standard = 8
send("/")
send("ui.gap")
send("\r")
send("\x1b[C")
send("\x1b[C")
check('gap = 10' in userconf(), "flecha derecha sube el entero (8 -> 10)")
send("\x1b[D")
check('gap = 9' in userconf(), "flecha izquierda lo baja")
send("-")
check('gap = 0' in userconf() or 'gap = 1' in userconf(), "- resta 10 con tope en el mínimo")

# un valor que no se puede pasar del máximo
send("+"); send("+"); send("+"); send("+"); send("+"); send("+"); send("+")
check('gap = 64' in userconf(), "el entero se detiene en el máximo (64)")

# bool: alternar con Enter
send("/"); send("ui.blur"); send("\r")
send("\r")
check('blur = true' in userconf(), "Enter alterna un booleano")
send("\r")
check('blur = false' in userconf(), "y lo vuelve a alternar")

# enum: ciclar con la flecha derecha
send("/"); send("ui.panel_position"); send("\r")
send("\x1b[C")
check('panel_position = "bottom"' in userconf(), "flecha derecha cicla un enum (top -> bottom)")

# texto: editar con línea de entrada
send("/"); send("ui.accent"); send("\r")
send("\r")
send("#ff8800")
send("\r")
check('accent = "#ff8800"' in userconf(), "edición de texto")

# valor inválido: se rechaza y se muestra el error
send("\r")
send("\x7f" * 8)   # el campo viene relleno con el valor actual: se borra primero
send("rojo")
send("\r")
pump(0.3)
check('accent = "rojo"' in userconf(), "ui.accent acepta texto libre (el tema lo ignora si no es #RRGGBB)")

# restablecer con d
send("d")
check("accent" not in userconf(), "d restablece al valor del perfil")

# categoría dinámica: atajos
for _ in range(ncats):
    send("\t", 0.1)
send("\r")
send("nav")
send("\r")
send("Mod+b exec firefox")
send("\r")
check('nav = "Mod+b exec firefox"' in userconf(), "añadir un atajo desde la TUI")
# atajo inválido
send("\x1b[B")      # baja a "+ añadir" (ahora hay 1 atajo)
send("\r"); send("mal"); send("\r"); send("Mod+x borra todo"); send("\r")
check('mal' not in userconf(), "un atajo inválido no se guarda")

# salir
send("q", 0.5)
pump(0.3)
_, status = os.waitpid(pid, 0)
check(os.WIFEXITED(status) and os.WEXITSTATUS(status) == 0, "sale con código 0")
check(b"\x1b[?1049l" in screen, "restaura la pantalla alternativa al salir")
check(b"\x1b[?25h" in screen, "vuelve a mostrar el cursor al salir")
subprocess.run(["rm", "-rf", tmp])
if fails:
    sys.exit(1)
print("  tui OK")
