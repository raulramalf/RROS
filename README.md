# RROS

Sistema operativo ligero y muy personalizable. Usa el **kernel Linux** con una base mínima de **Ubuntu** o **Arch Linux**; los componentes propios de RROS están escritos en **C**.

## Objetivos

| Objetivo | Meta |
|---|---|
| Perfiles de hardware | Lite / Standard / Pro (techo de diseño: 8 GB de RAM) |
| Disco | ≤ 20 GB (Lite ≤ 6, Standard ≤ 12, Pro ≤ 20) |
| RAM del SO en reposo | ≤ 4 GB (Lite < 512 MB, Standard < 2 GB, Pro ≤ 4 GB) |
| Arquitecturas | x86_64 (Intel/AMD) y ARM64 |
| Personalización | 96 ajustes por capas, 7 temas (más los tuyos), atajos, reglas por aplicación y programas de inicio propios ([docs/CUSTOMIZATION.md](docs/CUSTOMIZATION.md)). Escritorio, red, memoria y servicios ya se aplican; el resto se valida |
| Interfaz de ajustes | Ventana gráfica local (`rros settings`) con temas, tienda, seguridad, dispositivos y creación de archivos, y versión de terminal para Lite o SSH |
| Móvil | Android en modelos concretos; iPhone no viable ([docs/MOBILE.md](docs/MOBILE.md)) |

## Probarlo
- **Navegador:** demo del escritorio en [`preview/index.html`](preview/index.html) y de la ventana de ajustes (`scripts/build-gui-demo.sh` genera `build/gui-demo.html`).
- **Contenedor:** `scripts/try-docker.sh lite` (identidad y herramientas).
- **Máquina virtual:** ISO Lite real (`scripts/make-iso.sh lite`, ~1,4 GB) que arranca en QEMU con escritorio y aplicaciones; también se puede construir en GitHub Actions (workflow *ISO*, aún sin ejecutar allí). Ver [docs/TRY.md](docs/TRY.md).

![RROS Lite arrancado en QEMU: panel, terminal, calculadora, editor y archivos](docs/img/rros-lite-escritorio.png)

*Captura real de la ISO Lite arrancada en QEMU (sin KVM), tema Aurora.*

![RROS instalado en disco: terminal y RROS Ajustes en Epiphany](docs/img/rros-lite-instalado-ajustes.png)

*Captura real del sistema ya instalado en un disco virtual con `rros-install`: inicio de sesión en tty1, escritorio, red por cable y la ventana de Ajustes.*

![RROS Ajustes en la página de Seguridad, dentro del sistema live](docs/img/rros-seguridad.png)

*Captura real: `rros settings open seguridad` en la ISO Lite (QEMU). Auditoría con puntuación, cortafuegos activo y kernel endurecido.*

![RROS Ajustes en la página de Dispositivos, dentro del sistema live](docs/img/rros-dispositivos.png)

*Captura real: `rros settings open dispositivos`. El CD del sistema live no ofrece «Expulsar» porque está en uso.*

![Thunar mostrando ~/Plantillas y las carpetas del usuario en español](docs/img/rros-plantillas.png)

*Captura real: las plantillas de `rros nuevo` en `~/Plantillas` (el menú «Crear documento» del gestor de archivos) y las carpetas del usuario en español. Los menús de las aplicaciones siguen en inglés: faltan los paquetes de idioma.*

## Estado

| Pieza | Estado |
|---|---|
| `rros-hwdetect`, `rros-config`, `rros-budget`, `rros-installer plan` | ✅ hechas y probadas |
| `rros-theme` – 7 temas y temas propios; sway, waybar, foot, mako, fuzzel y GTK | ✅ hecha y probada |
| `rros-settings` – ventana gráfica web local (con token) y TUI | ✅ probadas (TUI en un pty, GUI con navegador) |
| Atajos, reglas por aplicación y autostart (`keybind.*`, `rule.*`, `autostart.*`) | ✅ validados igual que se generan; probados |
| Instantáneas, exportar e importar ajustes | ✅ hechas y probadas |
| `rros-apply` – aplica perfil a zram, swap, disco, red y servicios | ✅ hecha y probada (con raíz alternativa) |
| `rros-fetch`, `rros` (`doctor`, `info`, …) | ✅ hechas y probadas |
| Overlay de identidad (sin snap, sin telemetría, tty1 directo) | ✅ aplicado y comprobado sobre un directorio |
| ISO Lite: rootfs Ubuntu 24.04, GRUB, casper live, autotest | ✅ construida y arrancada en QEMU (BIOS, sin KVM): autotest de arranque, escritorio con panel y apps (captura arriba) |
| Aplicaciones incluidas (navegador, archivos, editor, visores, multimedia, red, Bluetooth, USB, bloqueo, energía) y tienda `rros-apps` | ✅ paquetes verificados contra Ubuntu noble; probadas las que se abrieron en la VM (terminal, calculadora, editor, archivos); el resto, sin abrir aún |
| Seguridad: cortafuegos, kernel endurecido, bloqueo por intentos, sudo, USB/cámara, actualizaciones; `rros seguridad` (auditoría y niveles) | ✅ probado con pruebas automáticas y en QEMU, incluido el bloqueo tras contraseñas fallidas ([docs/SEGURIDAD.md](docs/SEGURIDAD.md)) |
| Dispositivos: `rros dispositivos` (discos, USB, red, radios, sonido, pantallas, batería; montar, expulsar, Wi-Fi/Bluetooth) | ✅ probado con un /sys simulado y en QEMU (inventario real de la VM) |
| Creación de archivos: `rros nuevo` (plantillas, proyectos, lanzadores), «Crear documento» en Thunar, carpetas en español | ✅ probado |
| `rros-install` – instalador de texto a disco (BIOS, Lite/ext4) | ✅ probado en QEMU: instala, arranca desde el disco, login y escritorio |
| Instalación en UEFI y btrfs (Standard/Pro), instalador gráfico, hardware real, kernel por perfil, SwayFX, repositorio propio, móvil | ⏳ pendiente / sin probar |

La GUI es una aplicación web local: necesita un navegador (Standard y Pro lo incluyen; Lite usa la TUI). No hay todavía una ventana con toolkit nativo (GTK/Qt).

Medido en la ISO Lite: sistema de archivos 2,3 GB (límite 6) y, con el escritorio y cuatro apps abiertas en la VM, ~370 MB de RAM usada (límite 512 MB). Standard y Pro aún no se han construido ni medido; los IDs de Flatpak de la tienda no se han podido comprobar (Flathub no es alcanzable desde donde se construyó). Diferencias con Ubuntu: [docs/DIFFERENCES.md](docs/DIFFERENCES.md).

## Uso rápido

```
make                 # compila las herramientas (build/)
make test            # tests unitarios (ASan+UBSan) e integración
build/rros-hwdetect  # hardware detectado y perfil recomendado
build/rros-installer plan --disk-gb 64 --uefi -p standard
```

Personalizar (por usuario o `--system`). La guía completa de comandos, explicada paso a paso, está en [docs/CONSOLA.md](docs/CONSOLA.md):

```
rros config set ui.accent '#22d3ee'
rros theme use nord
rros config set keybind.nav 'Mod+Shift+b exec firefox'
rros settings              # ventana de ajustes (o `rros settings tui` en la terminal)
```

Sin instalar el sistema, desde el repositorio: `build/rros-config ...` con `RROS_PROFILE_DIR=profiles`.

Construir una ISO (en Ubuntu, como root; ver [docs/BUILD.md](docs/BUILD.md)).

## Estructura

```
src/lib/        conf (parser TOML), hw (detección), perfiles, esquema
src/hwdetect/   rros-hwdetect        src/config/   rros-config
src/budget/     rros-budget          src/installer/ rros-installer
src/theme/      rros-theme           src/apply/    rros-apply
src/fetch/      rros-fetch           src/rros/     rros
src/settings/   rros-settings (TUI y servidor web)    gui/   interfaz web (index.html, mock.js de la demo)
overlay/        archivos que dan identidad al sistema (common, ubuntu)
preview/        demo interactiva en el navegador
profiles/       lite.toml, standard.toml, pro.toml
kernel/         fragmentos de configuración del kernel por perfil
packages/       listas de paquetes (Ubuntu y Arch) por perfil
scripts/        rootfs, kernel, ISO, QEMU
dist/systemd/   rros-firstboot.service (elige el perfil en el primer arranque)
tests/          unitarios en C + integración (cli.sh)
docs/           arquitectura, perfiles, build, personalización, móvil, hoja de ruta
```
