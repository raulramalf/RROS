# Lily 2.0 «Lirio»

Sistema operativo ligero, en español y con diseño propio, sobre **Arch Linux** (Wayland). Mezcla lo mejor de Windows y de macOS: una **barra de tareas** siempre visible con menú de inicio, buscador, aplicaciones en el centro, bandeja y reloj (seis diseños: Fusión, Clásica, Isla, Dock, Doble y Lateral), con el acabado y la calma de macOS. El escritorio es **Lily Shell** (Qt Quick); de KDE Plasma solo queda KWin por debajo. Trae **antivirus** (ClamAV con tiempo real, análisis programados y cuarentena) y **directivas de seguridad** personalizables, una **Tienda** con todo Flathub y los repositorios de Arch, y **Ajustes de Lily**, la aplicación de ajustes del sistema. Colores del tema **Lirio**, los del logotipo.

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

Todo se puede cambiar después en Ajustes de Lily (efectos, dock 2D/3D, estilo del escritorio...). Sin aceleración gráfica (una máquina virtual sin 3D), Lily apaga solo los efectos para que vaya fluido. Las medidas reales están en [Estado](#estado).

## Probarlo

### En una máquina virtual o en un PC (el sistema completo)
La ISO la construye GitHub Actions: pestaña *Actions* → workflow **Arch** → la última ejecución en verde → *Artifacts* → `lily-lite-iso` (un .zip con la ISO dentro). Para otra edición: *Run workflow* y elige `standard` o `pro`.

- **VirtualBox:** tipo Linux / Arch Linux (64-bit), 4 GB de RAM (mínimo 2), 2 procesadores o más, 20 GB de disco. En *Pantalla*: controlador **VMSVGA**, 128 MB de vídeo y **Habilitar aceleración 3D** (sin ella, Lily va bien pero sin efectos). Si sale *AMD-V/VT-x is disabled*, activa la virtualización en la BIOS del PC ([docs/TRY.md](docs/TRY.md)).
- **PC real:** grábala en una memoria USB con balenaEtcher o Rufus (modo DD) y desactiva Secure Boot. Guía: [docs/PROBAR-EN-PC.md](docs/PROBAR-EN-PC.md).

Arranca en el escritorio sin tocar el disco y abre la **bienvenida**; desde ahí (o desde el menú Lily) se instala: junto a Windows en el espacio libre o en todo el disco, con cifrado si quieres.

### En el navegador (solo una demostración)
La carpeta [`site/`](site/) es una web estática (se publica en Vercel tal cual: `vercel.json` sirve `site/`). `escritorio.html` imita el escritorio con ventanas que se abren, arrastran y cierran. No es el sistema: el sistema de verdad es la ISO.

## Qué trae

- **Novedades de la 2.0**: barra de tareas en seis diseños y menú de inicio; barra superior de ordenador (bandeja agrupada, avisos, fecha); modo claro legible; letras en desplegable y punteros propios (Lily Lirio, Marfil y Grafito); seis fondos nuevos (pétalos, dunas, malla, cristal, topográfico y seda); iconos ilustrados; antivirus y directivas; Tienda completa.
- **Lily Shell** (`shell/`), el escritorio: barra superior (menú Lily, aplicación activa, Wi-Fi, sonido, batería, centro de control y reloj con calendario), dock 2D o 3D con lupa, rebote al abrir, puntos de las apps abiertas, apps fijadas (`ui.dock_apps`) y papelera; Launchpad a pantalla completa con buscador (también con la tecla Windows/Cmd); centro de control (Wi-Fi, Bluetooth, modo oscuro, no molestar, luz nocturna, captura, brillo y volumen); avisos de las aplicaciones y del volumen y el brillo. Cada pieza va en su capa de Wayland sobre KWin.
- **Ajustes de Lily**, una aplicación nativa (Qt Quick) con barra lateral, como las preferencias de macOS: General (este equipo y las ediciones), Apariencia, Escritorio y Dock, Red e Internet (Wi-Fi, redes ocultas, IP manual, DNS, MTU, MAC aleatoria, datos limitados, VPN, punto de acceso, proxy), Bluetooth, Sonido (salida y entrada, volumen, balance, conector, perfil de la tarjeta y volumen de cada aplicación), Pantallas (resolución, frecuencia, escala, giro, brillo, luz nocturna), Ratón y panel táctil (cada aparato), Teclado y atajos (y reglas de ventanas y programas de inicio), Batería y energía, Dispositivos, Fecha e idioma, Accesibilidad, Seguridad y privacidad, **Tienda**, Actualizaciones, Avanzado (los más de 130 ajustes uno a uno, copias y restablecer) e **Instalar Lily**.
- **Diseño propio**: tema de escritorio Lily en 2D (plano y translúcido) o 3D (brillo, bisel y canto, como un estante de cristal), barras de título con los botones de semáforo, menú Lily, iconos SVG propios para las aplicaciones de Lily (las demás llevan su logotipo de siempre), pantalla de carga, inicio de sesión y menú de arranque de Lily, y fondos generados con los colores de tu tema. El tema por defecto, **Lirio**, usa los colores del logotipo: vino oscuro, rojo lirio y champán.
- **Tres estilos de escritorio**: Moderno (el de Lily), Clásico (barra de tareas abajo, como Windows) y Enfoque (dock escondido, escritorio limpio).
- **Rendimiento**: efectos según la edición y la gráfica, el indexador de archivos apagado, sesiones que empiezan limpias, zram, y en el live las aplicaciones se precargan con 3,5 GB o más.
- **Seguridad**: cortafuegos, kernel endurecido, AppArmor, bloqueo tras contraseñas fallidas y auditoría con puntuación ([docs/SEGURIDAD.md](docs/SEGURIDAD.md)).
- **Consola**: el comando `rros` hace lo mismo que Ajustes ([docs/CONSOLA.md](docs/CONSOLA.md)).

## Estado

| Pieza | Estado |
|---|---|
| Herramientas en C (`rros-config`, `rros-theme`, `rros-apply`, `rros-apps`, `rros-security`, `rros-devices`, `rros-installer`, `rros-settings`…) | ✅ con pruebas automáticas (ASan + UBSan) |
| `rros-theme` para Plasma: barra y dock, efectos, entrada, energía, sonido, terminal, atajos, reglas e inicio; mezcla sus claves con las de KDE sin borrar las del usuario | ✅ probado |
| Lily Shell (Qt/QML) | ✅ en la ISO: sustituye a plasmashell y dibuja fondo, barra y dock (captura real en QEMU: [docs/img/rros-shell-live.png](docs/img/rros-shell-live.png)); Launchpad, centro de control, calendario y avisos revisados con `rros-shell --demo` |
| Ajustes de Lily (Qt/QML) | ✅ compila en la CI de Arch; todas las páginas abren sin errores (probado con Qt 6.4); red, sonido, Bluetooth y pantallas necesitan el sistema real |
| Tema 2D/3D, menú Lily, carga, inicio de sesión (SDDM), GRUB | ✅ tema y SDDM revisados con capturas fuera de Plasma; en el sistema, ver la CI |
| ISO Lite: arranque, autotest, instalación BIOS, UEFI y cifrada | ✅ (142–143 comprobaciones). Medido en QEMU con 2 GB y 2 hilos: live 950 MB de RAM y 1 % de CPU en reposo; instalado 560–600 MB y 3,6–4 %; ocupa 5,2 GB |
| Hardware real, Secure Boot, ediciones Standard y Pro medidas | ⏳ pendiente |

## Uso rápido

```
make                 # herramientas en C (build/)
make app             # Ajustes de Lily (necesita cmake, qt6-base y qt6-declarative)
make shell           # Lily Shell (SHELL_FULL=ON exige layer-shell-qt, kwindowsystem y kglobalaccel)
build/shell/rros-shell --demo        # el escritorio en una ventana, para probarlo
make test            # pruebas unitarias e integración
rros config set ui.dock_style 3d     # el dock en 3D
rros theme escritorio clasico        # estilo clásico
rros settings                        # abre Ajustes de Lily
```

Sin instalar, desde el repositorio: `build/rros-config ...` con `RROS_PROFILE_DIR=profiles`. Construir la ISO: [docs/BUILD.md](docs/BUILD.md).

## Estructura

```
src/            herramientas en C: lib (configuración, esquema, hardware), theme (Plasma), settings (API y TUI)...
app/            Ajustes de Lily: aplicación Qt Quick (main.cpp, backend.cpp, qml/, icons/)
shell/          Lily Shell: el escritorio (barra, dock, Launchpad, centro de control, avisos, fondo)
overlay/        lo que se añade al sistema: common y arch (tema de escritorio, menú Lily, SDDM, GRUB, iconos)
profiles/       lite.toml, standard.toml, pro.toml
packages/       paquetes de Arch por edición
apps/           catálogo de la Tienda (pacman y Flathub)
scripts/        ISO (archiso), QEMU, logotipo, iconos y tema de escritorio (make-logo.py, make-icons.py, make-desktoptheme.py)
site/           demostración en el navegador
tests/          unitarios en C, integración (cli.sh) y TUI
docs/           logotipo (brand/), consola, personalización, seguridad, ediciones, construir, probar en un PC
```
