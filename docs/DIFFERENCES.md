# Qué hace distinto a RROS

RROS usa el kernel Linux, los paquetes de **Arch Linux** y el escritorio **KDE Plasma**. Lo que lo distingue es lo
que se construye encima: no es «Arch con KDE», es un sistema con su propio aspecto, su aplicación de ajustes y sus
herramientas.

## Frente a un Arch con KDE Plasma recién instalado
| | Arch + Plasma | RROS |
|---|---|---|
| Aspecto | Breeze: barra de tareas abajo | Barra fina arriba con el menú RROS y el menú de la aplicación, dock 2D o 3D con Launchpad, botones de semáforo, tema de escritorio, colores, carga, inicio de sesión y GRUB propios |
| Ajustes | Preferencias del sistema de KDE (muy completas, muy densas) | **RROS Ajustes**: una ventana propia, en español sencillo, con lo importante de red, sonido, pantallas, dispositivos, energía, seguridad y apariencia; las Preferencias de KDE siguen ahí para lo más avanzado |
| Personalización | Por la interfaz de KDE | Además, más de 130 ajustes por capas con validación (`rros config`), temas propios, atajos, reglas de ventanas, copias de los ajustes |
| Rendimiento | Efectos de KDE por defecto | Ediciones Lite/Standard/Pro; efectos apagados sin aceleración gráfica; Baloo apagado; sesiones limpias |
| Tienda | Discover | Tienda de RROS: catálogo probado, pacman o Flathub, con el progreso en la ventana |
| Instalación | archinstall (en la consola) | Asistente gráfico: junto a Windows, cifrado, BIOS y UEFI |
| Seguridad | Lo que configures | Cortafuegos, kernel endurecido, AppArmor, bloqueo tras contraseñas fallidas y auditoría con puntuación |
| Idioma | Inglés hasta que lo cambies | Español en todo: menús, ajustes, instalador, consola y teclado |

## Frente a Ubuntu de escritorio
| | Ubuntu | RROS |
|---|---|---|
| Estabilidad | LTS: 5 años, pocos cambios | Rolling release: programas y kernel nuevos antes; alguna actualización puede pedir atención |
| Paquetes | apt y Snap | pacman y Flatpak; sin Snap |
| Telemetría | Informes de errores y Ubuntu Pro | Ninguna |
| Secure Boot | Sí | Hay que desactivarlo |
| Hardware | Probado en miles de equipos | Kernel y firmware más recientes, pero probado en máquinas virtuales; `rros informe` ayuda a probar en un PC real |

## Igual que Arch Linux
Kernel, firmware, pacman y sus repositorios, systemd, NetworkManager, PipeWire, mkinitcpio, GRUB y las
aplicaciones de KDE.
