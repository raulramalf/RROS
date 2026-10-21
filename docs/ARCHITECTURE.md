# Arquitectura

Lily usa el **kernel Linux**, los paquetes de **Arch Linux** y el escritorio **KDE Plasma** (Wayland), y escribe
lo que le da identidad: las herramientas en C, la aplicación de ajustes en Qt/QML y el diseño.

| Capa | Origen |
|---|---|
| Kernel y paquetes | Arch Linux (rolling): kernel `linux`, firmware, pacman |
| Escritorio | KDE Plasma 6 (Wayland) con el aspecto de Lily: tema de escritorio 2D/3D, barras de título, menú Lily, colores, iconos, carga, inicio de sesión y GRUB |
| **rros-config** | **C.** Ajustes por capas: perfil < sistema (`/etc/rros/rros.toml`) < usuario (`~/.config/rros/rros.toml`), validados con un esquema |
| **rros-theme** | **C.** Traduce los ajustes a Plasma: escribe sus claves en `kwinrc`, `kdeglobals`, `kcminputrc`, `powerdevilrc`... sin borrar las demás, genera la barra y el dock (guion de plasmashell), colores, barras de título, Konsole, atajos, reglas de ventanas y fondos |
| **rros-apply** | **C.** Lo que es de root: zram, red, cortafuegos, kernel, servicios, idioma |
| **rros-settings** | **C.** API en JSON (`rros-settings api`) que usa Ajustes de Lily, y versión de terminal (TUI) |
| **Ajustes de Lily** (`rros-ajustes`) | **C++/QML.** Ajustes, Tienda, bienvenida e instalador. Llama a la API y a las herramientas del sistema (nmcli, pactl, bluetoothctl, kscreen-doctor, KWin por D-Bus); lo de root pasa por pkexec |
| **rros-apps** | **C.** Tienda: pacman o Flathub |
| **rros-security**, **rros-devices**, **rros-new**, **rros-hwdetect**, **rros-budget** | **C.** Seguridad, dispositivos, plantillas, hardware y presupuestos |
| **rros-installer** / **rros-install** | **C** (plan de particiones) y **shell** (instalación: particiones, cifrado, copia, GRUB) |

## Presupuesto de recursos
Disco ≤ 20 GB (Lite 10, Standard 14, Pro 20) y RAM del sistema en reposo (Lite 1,3 GB, Standard 2 GB, Pro 3 GB).
El autotest mide la RAM, el disco y el procesador en reposo en cada ISO.

## Compatibilidad de hardware
La del kernel Linux y `linux-firmware`: Intel y AMD (x86-64). NVIDIA con el controlador libre; el propietario se
instala aparte. Secure Boot no está soportado (Arch no firma su arranque).
