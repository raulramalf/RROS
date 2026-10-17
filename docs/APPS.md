# Qué trae RROS: aplicaciones y sistema

Todo lo de esta página sale de `packages/arch-*.list`, y `scripts/check-packages-arch.sh` comprueba en la CI que cada paquete existe en los repositorios de Arch (las listas de la base Ubuntu, `packages/ubuntu-*.list`, se comprueban con `scripts/check-packages.sh`). **Nada de esto usa snap.**

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
| Tienda de aplicaciones | `rros apps` y la ventana de ajustes (pacman y Flatpak/Flathub) | |
| Ajustes | `rros settings` | `Mod+,` |
| Instalador | RROS Ajustes → *Instalar RROS* o `rros install` (solo en la ISO) | |

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
- Firmware de Linux para Wi-Fi, GPU, sonido de portátiles (SOF) y demás hardware, y microcódigo de Intel y AMD.
- Aplicaciones por defecto ya asociadas a cada tipo de archivo.

## Lo que añaden Standard y Pro

| | Standard | Pro |
|---|---|---|
| Procesador de textos y hoja de cálculo | AbiWord, Gnumeric | LibreOffice (Writer, Calc, Impress) |
| Correo | Geary | Geary |
| Música | Audacious | Audacious y VLC |
| Imágenes | | GIMP |
| Escáner | Simple Scan | Simple Scan |
| Impresión | CUPS y configuración de impresoras | CUPS y configuración de impresoras |
| Discos y sistema | Monitor del sistema, Discos | Monitor del sistema, Discos, GParted |
| Códecs | GStreamer completo y FFmpeg | GStreamer completo y FFmpeg |
| Desarrollo | Git, Python 3 | Git, Python 3, compiladores (base-devel), Vim |
| Carpetas de red y móviles | gvfs-mtp y gvfs-smb | gvfs-mtp y gvfs-smb |
| Vídeo acelerado y Vulkan | Intel y AMD | Intel y AMD |

## Instalar más (54 aplicaciones en el catálogo)

```sh
rros apps list                        # el catálogo y su estado
rros apps list --category games       # solo una categoría
rros apps search navegador            # buscar
rros apps info firefox                # detalles
rros apps install firefox             # instalar (pacman, o Flatpak si no hay paquete)
rros apps install libreoffice --flatpak
rros apps remove vlc
rros apps update                      # actualiza sistema y aplicaciones (también el botón «ACT» del panel)
```

También se puede hacer con un clic desde `rros settings`, en **Tienda de aplicaciones**: abre una terminal para que veas el progreso y puedas escribir tu contraseña.

Categorías: Internet (Firefox, Chromium, Brave, Thunderbird, Telegram, Signal, Discord...), Oficina (LibreOffice, calibre, Obsidian...), Multimedia (VLC, OBS Studio, Kdenlive, HandBrake, Audacity, Spotify...), Gráficos (GIMP, Inkscape, Krita, Blender...), Desarrollo (VS Code, Geany, Neovim, Docker, Node.js...), Sistema (Timeshift, BleachBit, máquinas virtuales...), Juegos (Steam, Lutris, SuperTuxKart) y Utilidades (KeePassXC, Bitwarden...).

Los paquetes de pacman los comprueba la CI de Arch (y los de apt, contra Ubuntu 24.04). Los identificadores de Flatpak son los de Flathub, pero no se han podido comprobar sin red.

En Arch se instala siempre con `pacman -Syu --needed`: actualizar solo una parte del sistema puede romperlo. Firefox, Chromium, Thunderbird, LibreOffice, Telegram, Signal, Discord, Obsidian y VS Code son paquetes normales de Arch, con su traducción al español cuando la hay; Brave, Spotify, Steam, Bitwarden... vienen de Flathub. En la base Ubuntu, Firefox y Chromium también salen de Flathub, porque allí son un envoltorio de snap.

## Cambiar las aplicaciones por defecto

```sh
rros config set apps.browser firefox      # Mod+B abrirá Firefox
rros config set apps.files   pcmanfm
rros config set apps.editor  geany
rros config set apps.terminal kitty
rros theme apply
```
Para el tipo de archivo (qué abre un PDF, por ejemplo): `xdg-mime default org.gnome.Evince.desktop application/pdf`.
