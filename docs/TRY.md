# Probar RROS

Hay tres formas, de la más rápida a la más real.

## 1. En el navegador (sin instalar nada)
La demo interactiva está en [`preview/index.html`](../preview/index.html). Ábrela con doble clic o sírvela con `python3 -m http.server -d preview`.
Simula el escritorio: perfiles, tema, panel, acento y una terminal con `rros-fetch`, `rros-config`, `rros doctor` y `rros-installer plan`. Los valores de memoria y disco son de ejemplo.

### La ventana de ajustes en el navegador
`scripts/build-gui-demo.sh` genera `build/gui-demo.html`: la misma interfaz de `rros-settings` con un servidor simulado. Permite elegir temas, crear uno propio y editar sus colores, añadir atajos y reglas, y guardar instantáneas.

### La ventana de ajustes de verdad
Con las herramientas compiladas (`make`) puedes lanzarla en tu propio equipo, sin instalar RROS:
```
export RROS_PROFILE_DIR=$PWD/profiles RROS_GUI_DIR=$PWD/gui
build/rros-settings web          # imprime una URL con token y abre tu navegador
build/rros-settings tui          # la versión de terminal
```
Sin `RROS_USER_CONF`, escribe en tu `~/.config/rros/rros.toml`. Para probar sin tocarlo: `RROS_USER_CONF=/tmp/prueba.toml`.

## 2. En un contenedor (identidad y herramientas reales)
```
scripts/try-docker.sh lite     # o standard, pro
```
Abre una consola con la identidad de RROS, el prompt, el banner (`rros-fetch`), `rros doctor` y todas las herramientas en C compiladas. No arranca escritorio ni kernel.

## 3. ISO en una máquina virtual (el sistema completo)
**Sin tocar tu PC:** en GitHub, pestaña *Actions*, workflow **Arch**, *Run workflow*, elige el perfil. Al terminar,
descarga el artefacto `rros-arch-<perfil>-iso` (y el registro del autotest de arranque).

**En un Arch:** `sudo scripts/build-iso-arch.sh lite` (ver [BUILD.md](BUILD.md)). La ISO de la base Ubuntu se
construye en Ubuntu con `scripts/make-iso.sh`.

En una máquina virtual sin aceleración gráfica el escritorio es sway (el de respaldo); Hyprland, con sus
animaciones y desenfoque, necesita una GPU: en QEMU con `-device virtio-vga-gl -display gtk,gl=on`, o en un PC.

Arranque en QEMU:
```
scripts/run-qemu.sh lite            # BIOS
scripts/run-qemu.sh lite --uefi     # UEFI (requiere OVMF)
```
En VirtualBox o GNOME Boxes: tipo Linux/Arch Linux 64 bits, la RAM del perfil (Lite 1 GB, Standard 4 GB, Pro 8 GB), activa EFI si quieres y monta la ISO. El sistema live entra solo con el usuario `rros` en tty1 y arranca el escritorio.

## 4. Instalarlo en un disco
Desde el sistema live (la ISO), `rros-install` instala RROS en un disco: lo particiona según el perfil (`rros-installer plan`), formatea, copia el sistema, crea tu usuario, configura teclado y zona horaria e instala GRUB.
```
rros install --disk /dev/vda --dry-run              # solo muestra lo que haría
rros install                                         # asistente: pregunta disco, usuario y contraseña
rros install --disk /dev/vda --user ana --hostname mi-pc --keymap es --timezone Europe/Madrid \
             --password-file /tmp/pw --yes          # sin preguntas (para scripts)
```
Borra **todo** el disco elegido y pide escribir su ruta para confirmar (salvo con `--yes`); `--alongside` instala en el espacio libre junto a otro sistema y `--encrypt` cifra con LUKS2. También hay asistente gráfico en RROS Ajustes → *Instalar RROS*.
Estado: con la base Arch, la CI lo prueba en cada ISO de principio a fin en QEMU: instala sin preguntas en un disco virtual (`scripts/test-iso.sh lite install`), arranca desde el disco y el sistema instalado pasa el autotest; también cifrado con LUKS2 (la prueba escribe la contraseña en la consola al arrancar). Con la base Ubuntu se probó en QEMU con BIOS y UEFI, cifrado y junto a un Windows simulado.

## Qué mirar al arrancar
- `rros-fetch` y `rros doctor`: identidad, perfil y presupuesto.
- `Mod+T` alterna oscuro/claro, `Mod+Shift+T` cambia de tema, `Mod+Alt+W` cambia el fondo, `Mod+,` abre los ajustes, `Mod+P` la ficha y `Mod+D` el lanzador. `Mod+F1` muestra todos los atajos.
- La guía de comandos está en [CONSOLA.md](CONSOLA.md).
- `rros config set ui.accent '#22d3ee'` y `rros theme apply` cambian el escritorio.
