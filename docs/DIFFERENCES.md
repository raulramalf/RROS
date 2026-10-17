# Qué hace distinto a RROS

RROS usa el kernel Linux y los paquetes de **Arch Linux** como base, con **Hyprland** como escritorio. Lo que lo distingue es lo que se construye encima. (La primera base fue Ubuntu 24.04; sigue en el repositorio como base secundaria, pero lo que se desarrolla y se prueba ahora es Arch.)

## Distinto de verdad
| Aspecto | Ubuntu 24.04 | RROS |
|---|---|---|
| Escritorio | GNOME Shell | **Hyprland** (animaciones, desenfoque, bordes con degradado, sombras, mosaico dwindle o master, gestos del panel táctil) con waybar, todo generado por `rros-theme` desde tus ajustes. Sin GPU (máquinas virtuales), sway como respaldo |
| Estilos de escritorio | Uno | **Clásico** (barra abajo con botón Inicio y lista de ventanas), **Moderno** (barra flotante arriba y dock) y **Mosaico** (ventanas en mosaico, barra con escritorios); se cambian con un clic |
| Fondo de pantalla | Imágenes fijas | **Generado con los colores de tu tema** (aurora, ondas, degradado, círculos o liso), o tu imagen; Super+Alt+W pasa al siguiente |
| Inicio de sesión | Gestor de pantalla | Directo desde tty1, sin gestor; bloqueo con hyprlock |
| Paquetes | apt y Snap | **pacman** (rolling release: siempre lo último) y Flatpak; sin Snap |
| Actualizaciones | Las de seguridad se instalan solas | Se **descargan** solas cada día y las instalas tú (`rros apps update`): en Arch, actualizar sin mirar puede romper cosas |
| Telemetría | Informes de errores (apport) y Ubuntu Pro | Ninguna |
| Memoria | Swap en archivo | zram según perfil (`rros-apply`) |
| Personalización | dconf y ajustes gráficos | TOML por capas con 129 claves validadas, 7 temas y temas propios, atajos, reglas por aplicación y programas de inicio |
| Ajustes | GNOME Settings | `rros-settings`: ventana propia que usa el tema que editas, y versión de terminal |
| Tema | Un tema y el modo oscuro | Un solo tema se aplica a Hyprland, waybar, terminal, notificaciones, lanzador, bloqueo, fondo y GTK; se puede crear el propio |
| Presupuesto | Ninguno | RAM y disco con tope por perfil (`rros-budget`) |
| Perfiles | Una sola variante | Lite, Standard y Pro que cambian efectos, servicios, memoria y aplicaciones |
| Herramientas | `apt`, `snap`, `gsettings` | Comando `rros`: `info`, `config`, `theme`, `apps`, `seguridad`, `doctor`, `budget`, `hw`, `install` |
| Identidad | `ID=ubuntu` | `ID=rros` (`ID_LIKE=arch`), tema Aurora, banner, prompt y GRUB propios |

## Igual que Arch Linux
Kernel (`linux`, el estable más reciente) y firmware, pacman y sus repositorios, systemd, NetworkManager, PipeWire, mkinitcpio, GRUB y los programas (navegador, archivos, editor…) que son los de siempre del escritorio Linux. El kernel propio recortado por perfil está definido pero sin compilar.

## En lo práctico, frente a Ubuntu 24.04 de escritorio

| | Ubuntu | RROS (Arch) |
|---|---|---|
| Estabilidad | LTS: 5 años de soporte, pocos cambios | Rolling release: programas y kernel nuevos antes, pero más cambios y alguna actualización puede necesitar intervención |
| Instalador | Gráfico (Subiquity/Flutter); cifrado; junto a Windows, también reduciendo su partición | Gráfico en RROS Ajustes → *Instalar RROS* (y de texto, `rros install`); cifrado LUKS2 (sd-encrypt); junto a Windows **en el espacio libre** (hay que reducir el volumen desde Windows antes); BIOS y UEFI |
| Arranque | BIOS, UEFI y Secure Boot | BIOS y UEFI. **Secure Boot hay que desactivarlo**: Arch no firma su arranque (la base Ubuntu de RROS sí arranca con Secure Boot) |
| Seguridad | AppArmor, cortafuegos desactivado | AppArmor (activado en el arranque con `lsm=`), cortafuegos nftables activo, kernel endurecido, bloqueo tras contraseñas fallidas; auditoría con `rros seguridad` |
| Hardware | Probado en miles de equipos y certificado en muchos | Kernel y firmware más recientes que Ubuntu, pero **solo probado en QEMU**; `rros informe` y la [guía para probar en un PC](PROBAR-EN-PC.md) sirven para reunir pruebas reales |
| Idioma | Todo en español, incluido el instalador | Sistema, menús de las aplicaciones, ajustes, instalador, consola y teclado en español |
| Facilidad | GNOME: ratón primero | Ventanas libres o mosaico a elegir en la bienvenida, estilo clásico con botón Inicio y lista de ventanas, dock, ayuda de atajos (Super + F1) |
| Accesibilidad | Orca, teclado en pantalla, zoom, alto contraste, tamaños | Orca con voz en español, teclado en pantalla, alto contraste, tamaño de letra y de cursor, menos animaciones. **Sin zoom de pantalla** |

## Qué falta para que el cambio se note más
Pruebas en hardware real (sobre todo Hyprland, que en una máquina virtual sin GPU no se puede ver), reducir una partición de Windows desde el instalador, zoom de pantalla, Secure Boot en la base Arch (firmar con claves propias), repositorio de paquetes de RROS y aplicar al sistema el resto de claves (audio, energía, arranque).
