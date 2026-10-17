# Qué hace distinto a RROS

RROS usa el kernel Linux y paquetes de Ubuntu 24.04 como base. Lo que lo distingue es lo que se construye encima.

## Distinto de verdad
| Aspecto | Ubuntu | RROS |
|---|---|---|
| Escritorio | GNOME Shell | **RROS Shell**: sway + waybar + foot, generados por `rros-theme` desde tus ajustes |
| Inicio de sesión | Gestor de pantalla | Directo desde tty1, sin gestor |
| Paquetes | Snap por defecto | Sin snap (apt pinning y servicios enmascarados) |
| Telemetría | Informes y servicios de Ubuntu Pro | Bloqueados y desactivados |
| Memoria | Swap en archivo | zram según perfil (`rros-apply`) |
| Personalización | dconf y ajustes gráficos | TOML por capas con 96 claves validadas, 7 temas y temas propios, atajos, reglas por aplicación y programas de inicio |
| Ajustes | GNOME Settings | `rros-settings`: ventana propia que usa el tema que editas, y versión de terminal |
| Tema | Un tema y el modo oscuro | Un solo tema se aplica a sway, waybar, terminal, notificaciones, lanzador y GTK; se puede crear el propio |
| Presupuesto | Ninguno | RAM y disco con tope por perfil (`rros-budget`) |
| Perfiles | Una sola variante | Lite, Standard y Pro que cambian compositor, efectos, servicios y memoria |
| Herramientas | `apt`, `snap`, `gsettings` | Comando `rros`: `info`, `config`, `theme`, `doctor`, `budget`, `hw`, `install` |
| Identidad | `ID=ubuntu` | `ID=rros`, tema Aurora, banner, prompt y GRUB propios |

## Igual que Ubuntu (de momento)
Kernel (el HWE de Ubuntu 24.04.4 de escritorio) y firmware, `apt` y los repositorios, systemd, NetworkManager, PipeWire, la cadena de arranque firmada (shim, GRUB y kernel) y los programas (navegador, archivos, editor…) que son los de siempre del escritorio Linux. El kernel propio recortado por perfil está definido pero sin compilar.

## En lo práctico, frente a Ubuntu 24.04 de escritorio

| | Ubuntu | RROS |
|---|---|---|
| Instalador | Gráfico (Subiquity/Flutter); cifrado; junto a Windows, también reduciendo su partición | Gráfico en RROS Ajustes → *Instalar RROS* (y de texto, `rros install`); cifrado LUKS2; junto a Windows **en el espacio libre** (hay que reducir el volumen desde Windows antes); BIOS y UEFI |
| Arranque | BIOS, UEFI y Secure Boot | Igual: la ISO usa el shim y el GRUB firmados de Ubuntu |
| Hardware | Probado en miles de equipos y certificado en muchos | Mismo kernel, firmware y controladores que Ubuntu, pero **solo probado en QEMU**; `rros informe` y la [guía para probar en un PC](PROBAR-EN-PC.md) sirven para reunir pruebas reales |
| Idioma | Todo en español, incluido el instalador | Sistema, menús de las aplicaciones (paquetes de idioma), ajustes, instalador, consola y teclado en español |
| Facilidad | GNOME: ratón primero | Ventanas libres o mosaico a elegir en la bienvenida, barra de ventanas, menú «Aplicaciones», ayuda de atajos (Super + F1) |
| Accesibilidad | Orca, teclado en pantalla, zoom, alto contraste, tamaños | Orca con voz en español, teclado en pantalla, alto contraste, tamaño de letra y de cursor, menos animaciones. **Sin zoom de pantalla** |

## Qué falta para que el cambio se note más
Pruebas en hardware real, reducir una partición de Windows desde el instalador, zoom de pantalla, repositorio de paquetes de RROS, aplicar al sistema el resto de claves (audio, energía, arranque) y paquete de SwayFX para el compositor *full*.
