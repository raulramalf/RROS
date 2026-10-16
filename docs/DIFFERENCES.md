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
Kernel base, `apt` y los repositorios, systemd, NetworkManager, PipeWire, y el instalador (aún no hay `rros-installer` real). El kernel propio recortado por perfil está definido pero sin compilar.

## Qué falta para que el cambio se note más
Interfaz gráfica de ajustes, instalador propio, repositorio de paquetes de RROS, aplicar al sistema el resto de claves (audio, energía, arranque) y paquete de SwayFX para el compositor *full*.
