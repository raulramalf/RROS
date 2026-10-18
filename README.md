# RROS 1.0 «Boreal»

Sistema operativo ligero, en español y con diseño propio, sobre **Arch Linux** y **KDE Plasma** (Wayland). Se parece a macOS en lo que importa: barra fina arriba con el **menú RROS**, el menú de la aplicación y el reloj, y un **dock en 2D o en 3D** con el Launchpad. Lo demás es suyo: la aplicación **RROS Ajustes** (nativa, sin navegador), la Tienda, la bienvenida, el instalador, los iconos de sus aplicaciones, la pantalla de carga, el inicio de sesión y el menú de arranque. Los componentes propios están escritos en **C** y la aplicación de ajustes en **Qt/QML**.

## Ediciones: elige la de tu equipo

| | Lite | Standard | Pro |
|---|---|---|---|
| RAM del equipo | 2–4 GB | 4–8 GB | 8 GB |
| Procesador | 2 núcleos de 64 bits | 4 hilos | 4 núcleos (8 hilos) y GPU |
| Disco que ocupa | ≤ 10 GB | ≤ 14 GB | ≤ 20 GB |
| RAM del sistema en reposo (tope) | 1,3 GB | 2 GB | 3 GB |
| Efectos | ninguno: todo aparece al instante | animaciones rápidas | todos: desenfoque, lámpara mágica, transparencias |
| Dock | 2D | 2D | 3D |
| Aplicaciones | Firefox, archivos, terminal, editor, visores, música y vídeo | + LibreOffice, Elisa, impresoras y escáner, móviles | + GIMP, VLC, particiones, KDE Connect, desarrollo |

Todo se puede cambiar después en RROS Ajustes (efectos, dock 2D/3D, estilo del escritorio...). Sin aceleración gráfica (una máquina virtual sin 3D), RROS apaga solo los efectos para que vaya fluido. Las medidas reales están en [Estado](#estado).

## Probarlo

### En una máquina virtual o en un PC (el sistema completo)
La ISO la construye GitHub Actions: pestaña *Actions* → workflow **Arch** → la última ejecución en verde → *Artifacts* → `rros-arch-lite-iso` (un .zip con la ISO dentro). Para otra edición: *Run workflow* y elige `standard` o `pro`.

- **VirtualBox:** tipo Linux / Arch Linux (64-bit), 4 GB de RAM (mínimo 2), 2 procesadores o más, 20 GB de disco. En *Pantalla*: controlador **VMSVGA**, 128 MB de vídeo y **Habilitar aceleración 3D** (sin ella, RROS va bien pero sin efectos). Si sale *AMD-V/VT-x is disabled*, activa la virtualización en la BIOS del PC ([docs/TRY.md](docs/TRY.md)).
- **PC real:** grábala en una memoria USB con balenaEtcher o Rufus (modo DD) y desactiva Secure Boot. Guía: [docs/PROBAR-EN-PC.md](docs/PROBAR-EN-PC.md).

Arranca en el escritorio sin tocar el disco y abre la **bienvenida**; desde ahí (o desde el menú RROS) se instala: junto a Windows en el espacio libre o en todo el disco, con cifrado si quieres.

### En el navegador (solo una demostración)
La carpeta [`site/`](site/) es una web estática (se publica en Vercel tal cual: `vercel.json` sirve `site/`). `escritorio.html` imita el escritorio con ventanas que se abren, arrastran y cierran. No es el sistema: el sistema de verdad es la ISO.

## Qué trae

- **RROS Ajustes**, una aplicación nativa (Qt Quick) con barra lateral, como las preferencias de macOS: General (este equipo y las ediciones), Apariencia, Escritorio y Dock, Red e Internet (Wi-Fi, redes ocultas, IP manual, DNS, MTU, MAC aleatoria, datos limitados, VPN, punto de acceso, proxy), Bluetooth, Sonido (salida y entrada, volumen, balance, conector, perfil de la tarjeta y volumen de cada aplicación), Pantallas (resolución, frecuencia, escala, giro, brillo, luz nocturna), Ratón y panel táctil (cada aparato), Teclado y atajos (y reglas de ventanas y programas de inicio), Batería y energía, Dispositivos, Fecha e idioma, Accesibilidad, Seguridad y privacidad, **Tienda**, Actualizaciones, Avanzado (los más de 130 ajustes uno a uno, copias y restablecer) e **Instalar RROS**.
- **Diseño propio**: tema de escritorio RROS en 2D (plano y translúcido) o 3D (brillo, bisel y canto, como un estante de cristal), barras de título con los botones de semáforo, menú RROS, iconos SVG propios para las aplicaciones de RROS (las demás llevan su logotipo de siempre), pantalla de carga, inicio de sesión y menú de arranque de RROS, y fondos generados con los colores de tu tema.
- **Tres estilos de escritorio**: Moderno (el de RROS), Clásico (barra de tareas abajo, como Windows) y Enfoque (dock escondido, escritorio limpio).
- **Rendimiento**: efectos según la edición y la gráfica, el indexador de archivos apagado, sesiones que empiezan limpias, zram, y en el live las aplicaciones se precargan con 3,5 GB o más.
- **Seguridad**: cortafuegos, kernel endurecido, AppArmor, bloqueo tras contraseñas fallidas y auditoría con puntuación ([docs/SEGURIDAD.md](docs/SEGURIDAD.md)).
- **Consola**: el comando `rros` hace lo mismo que Ajustes ([docs/CONSOLA.md](docs/CONSOLA.md)).

## Estado

| Pieza | Estado |
|---|---|
| Herramientas en C (`rros-config`, `rros-theme`, `rros-apply`, `rros-apps`, `rros-security`, `rros-devices`, `rros-installer`, `rros-settings`…) | ✅ con pruebas automáticas (ASan + UBSan) |
| `rros-theme` para Plasma: barra y dock, efectos, entrada, energía, sonido, terminal, atajos, reglas e inicio; mezcla sus claves con las de KDE sin borrar las del usuario | ✅ probado |
| RROS Ajustes (Qt/QML) | ✅ compila en la CI de Arch; todas las páginas abren sin errores (probado con Qt 6.4); red, sonido, Bluetooth y pantallas necesitan el sistema real |
| Tema 2D/3D, menú RROS, carga, inicio de sesión (SDDM), GRUB | ✅ tema y SDDM revisados con capturas fuera de Plasma; en el sistema, ver la CI |
| ISO Lite: arranque, autotest, instalación BIOS, UEFI y cifrada | ver la última ejecución del workflow **Arch** |
| Hardware real, Secure Boot, ediciones Standard y Pro medidas | ⏳ pendiente |

## Uso rápido

```
make                 # herramientas en C (build/)
make app             # RROS Ajustes (necesita cmake, qt6-base y qt6-declarative)
make test            # pruebas unitarias e integración
rros config set ui.dock_style 3d     # el dock en 3D
rros theme escritorio clasico        # estilo clásico
rros settings                        # abre RROS Ajustes
```

Sin instalar, desde el repositorio: `build/rros-config ...` con `RROS_PROFILE_DIR=profiles`. Construir la ISO: [docs/BUILD.md](docs/BUILD.md).

## Estructura

```
src/            herramientas en C: lib (configuración, esquema, hardware), theme (Plasma), settings (API y TUI)...
app/            RROS Ajustes: aplicación Qt Quick (main.cpp, backend.cpp, qml/, icons/)
overlay/        lo que se añade al sistema: common y arch (tema de escritorio, menú RROS, SDDM, GRUB, iconos)
profiles/       lite.toml, standard.toml, pro.toml
packages/       paquetes de Arch por edición
apps/           catálogo de la Tienda (pacman y Flathub)
scripts/        ISO (archiso), QEMU, iconos y tema de escritorio (make-icons.py, make-desktoptheme.py)
site/           demostración en el navegador
tests/          unitarios en C, integración (cli.sh) y TUI
docs/           consola, personalización, seguridad, ediciones, construir, probar en un PC
```
