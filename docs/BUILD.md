# Construir Lily

## Herramientas en C (cualquier Linux)
```
make WERROR=1 && make test
```
Necesita `gcc` y `make`.

## Ajustes de Lily (Qt 6)
```
make app            # build/app/rros-ajustes
```
Necesita `cmake`, `qt6-base` y `qt6-declarative` (y en tiempo de ejecución `qt6-svg`). Sin instalar Lily, desde
el repositorio:
```
PATH=$PWD/build:$PATH RROS_PROFILE_DIR=$PWD/profiles build/app/rros-ajustes --pagina apariencia
build/app/rros-ajustes --bienvenida
build/app/rros-ajustes --pagina tienda --captura /tmp/tienda.png     # guarda una captura y sale
```

## Iconos y tema de escritorio
Son SVG generados (se guardan en el repositorio):
```
scripts/make-icons.py          # logotipo, aplicaciones de Lily y secciones de Ajustes
scripts/make-desktoptheme.py   # tema de escritorio de Plasma: rros (2D) y rros-3d
```

## ISO (Arch Linux)
Se construye con archiso en un Arch (o en el contenedor `archlinux`), como root:
```
pacman -S --needed archiso base-devel cmake qt6-base qt6-declarative
make && make app
sudo scripts/build-iso-arch.sh lite           # build/rros-arch-lite.iso
BASE=arch ISO=build/rros-arch-lite.iso scripts/test-iso.sh lite selftest   # autotest del live en QEMU
BASE=arch ISO=build/rros-arch-lite.iso scripts/test-iso.sh lite install    # instala en un disco virtual y lo arranca
CRYPT=1 BASE=arch ISO=build/rros-arch-lite.iso scripts/test-iso.sh lite install          # cifrado
FIRMWARE=uefi BASE=arch ISO=build/rros-arch-lite.iso scripts/test-iso.sh lite install   # UEFI (OVMF)
```
Sin un Arch a mano lo hace GitHub Actions: workflow **Arch** (*Run workflow*, o `[iso-arch]` en el mensaje de un
commit). Comprueba que todos los paquetes existen (`scripts/check-packages-arch.sh`, también los de la Tienda),
compila (también Ajustes de Lily), pasa las pruebas, construye la ISO, la arranca en QEMU con el autotest (y una
captura del escritorio) y la instala en un disco virtual (normal, UEFI y cifrada), arrancando cada instalación.

La ISO arranca en BIOS y UEFI, **sin Secure Boot**.

## Medir el presupuesto
```
build/rros-budget ram -p lite      # dentro del sistema en reposo
```
Sale con código 1 si se supera; pensado para CI.

## Variables de entorno (pruebas)
`RROS_PROFILE_DIR`, `RROS_SYSTEM_CONF`, `RROS_USER_CONF`, `RROS_PROFILE_FILE`, `RROS_ROOT` (raíz falsa),
`RROS_LIVE=1|0` (simula el sistema live), `RROS_NO_APPLY=1` (la API no aplica el tema),
`RROS_SOFT_RENDER=1|0` (simula una gráfica sin aceleración), `RROS_PLASMA_TEST=1` (rros-theme no llama a Plasma).
