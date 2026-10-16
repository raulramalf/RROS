# Qué trae RROS: aplicaciones y sistema

Todo lo de esta página sale de `packages/ubuntu-*.list`, y `scripts/check-packages.sh` comprueba que cada paquete existe en Ubuntu 24.04. **Nada de esto usa snap.**

## En todos los perfiles (incluido Lite)

| Para qué | Programa | Atajo |
|---|---|---|
| Navegar por Internet | Web (Epiphany) | `Mod+B` |
| Archivos, USB, papelera | Thunar (con miniaturas y comprimir/descomprimir) | `Mod+E` |
| Editar texto | Mousepad | `Mod+Shift+N` |
| Ver imágenes | imv | |
| Leer PDF | Evince | |
| Vídeo y música | mpv | |
| Comprimir y descomprimir | File Roller (zip, 7z, tar) | |
| Calcular | Galculator | |
| Procesos y memoria | btop (en la terminal) | |
| Terminal | foot | `Mod+Enter` |
| Lanzador de aplicaciones | fuzzel | `Mod+D` |
| Capturas de pantalla | grim y slurp | `Impr` / `Mod+Shift+S` |
| Historial del portapapeles | cliphist | `Mod+V` |
| Tienda de aplicaciones | `rros apps` y la ventana de ajustes (Flatpak/Flathub y apt) | |
| Ajustes | `rros settings` | `Mod+,` |
| Instalador | Calamares (solo en la ISO) | |

**Sistema que funciona sin tocar nada**

- Red y Wi-Fi: NetworkManager con applet en el panel (`Mod+Shift+W` abre las conexiones).
- Bluetooth: BlueZ con Blueman.
- Sonido: PipeWire con WirePlumber; el panel abre Pavucontrol.
- USB y discos: montaje automático con notificación (udiskie, udisks2, gvfs).
- Contraseñas: agente de polkit, para instalar programas o cambiar ajustes con ventana.
- Bloqueo, pantalla apagada y suspensión por inactividad (`Mod+Esc` bloquea). Los tiempos son ajustes.
- Menú de energía: bloquear, cerrar sesión, suspender, reiniciar, apagar (`Mod+Shift+E`).
- Notificaciones (mako), portales para diálogos de archivos y compartir pantalla, fuentes y emojis, tema GTK coherente.
- Memoria: zram comprimido. Energía: perfiles de rendimiento (power-profiles-daemon). Brillo: teclas de brillo.
- Firmware de Linux para Wi-Fi, GPU y demás hardware, y Secure Boot con `shim`.
- Aplicaciones por defecto ya asociadas a cada tipo de archivo.

## Lo que añaden Standard y Pro

| | Standard | Pro |
|---|---|---|
| Procesador de textos y hoja de cálculo | AbiWord, Gnumeric | LibreOffice (Writer, Calc, Impress) |
| Correo | Geary | Geary |
| Música | Audacious | Audacious y VLC |
| Imágenes | | GIMP |
| Escáner | Simple Scan | Simple Scan |
| Impresión | CUPS y configuración de impresoras | Lo mismo, con controladores de muchos fabricantes |
| Discos y sistema | Monitor del sistema, Discos | Monitor del sistema, Discos, GParted |
| Códecs | GStreamer completo y FFmpeg | GStreamer completo y FFmpeg |
| Desarrollo | Git, Python 3 | Git, Python 3, compiladores (build-essential), Vim |
| Carpetas de red y móviles | gvfs-backends | gvfs-backends |

## Instalar más (54 aplicaciones en el catálogo)

```sh
rros apps list                        # el catálogo y su estado
rros apps list --category games       # solo una categoría
rros apps search navegador            # buscar
rros apps info firefox                # detalles
rros apps install firefox             # instalar (apt, o Flatpak si no hay otra opción)
rros apps install libreoffice --flatpak
rros apps remove vlc
rros apps update                      # actualiza sistema y aplicaciones
```

También se puede hacer con un clic desde `rros settings`, en **Tienda de aplicaciones**: abre una terminal para que veas el progreso y puedas escribir tu contraseña.

Categorías: Internet (Firefox, Chromium, Brave, Thunderbird, Telegram, Signal, Discord...), Oficina (LibreOffice, calibre, Obsidian...), Multimedia (VLC, OBS Studio, Kdenlive, HandBrake, Audacity, Spotify...), Gráficos (GIMP, Inkscape, Krita, Blender...), Desarrollo (VS Code, Geany, Neovim, Docker, Node.js...), Sistema (Timeshift, BleachBit, máquinas virtuales...), Juegos (Steam, Lutris, SuperTuxKart) y Utilidades (KeePassXC, Bitwarden...).

Los paquetes de `apt` están comprobados contra Ubuntu 24.04. Los identificadores de Flatpak son los de Flathub, pero no se han podido comprobar sin red.

**Por qué Firefox y Chromium vienen por Flatpak:** en Ubuntu esos paquetes son un envoltorio de snap. RROS no usa snap, así que el navegador por defecto es Epiphany (un `.deb` real) y los demás se instalan desde Flathub.

## Cambiar las aplicaciones por defecto

```sh
rros config set apps.browser firefox      # Mod+B abrirá Firefox
rros config set apps.files   pcmanfm
rros config set apps.editor  geany
rros config set apps.terminal kitty
rros theme apply
```
Para el tipo de archivo (qué abre un PDF, por ejemplo): `xdg-mime default org.gnome.Evince.desktop application/pdf`.
