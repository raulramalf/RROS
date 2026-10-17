# Construir RROS

## Herramientas en C (cualquier Linux)
```
make WERROR=1 && make test
```
Necesita `gcc` y `make`.

## ISO sobre Arch Linux (la base principal)
Se construye con archiso en un Arch (o en el contenedor `archlinux`), como root:
```
pacman -S --needed archiso base-devel
make
sudo scripts/build-iso-arch.sh lite           # build/rros-arch-lite.iso
BASE=arch ISO=build/rros-arch-lite.iso scripts/test-iso.sh lite selftest   # autotest en QEMU
```
Sin un Arch a mano lo hace GitHub Actions: workflow **Arch** (al pulsar *Run workflow*, o con `[iso-arch]` en el
mensaje de un commit). Antes comprueba que todos los paquetes existen (`scripts/check-packages-arch.sh`, también los
de la tienda), pasa las pruebas y valida con el Hyprland real las configuraciones que genera `rros-theme`
(`scripts/check-hyprland.sh`). La ISO y el registro del autotest quedan como artefactos del workflow.

La ISO de Arch arranca en BIOS y UEFI, **sin Secure Boot**.

## ISO sobre Ubuntu 24.04 (base secundaria: Lite construida y arrancada en QEMU, también con Secure Boot)
Requisitos (Ubuntu/Debian): `debootstrap squashfs-tools grub-pc-bin grub-efi-amd64-bin xorriso mtools qemu-system-x86 ovmf`.
```
make
sudo scripts/build-rootfs-ubuntu.sh lite      # rootfs + presupuesto de disco
sudo scripts/make-iso.sh lite                 # build/rros-lite.iso
scripts/run-qemu.sh lite [--uefi]             # RAM acotada al perfil
```

Herramientas de prueba y depuración (probadas con la ISO Lite):
```
scripts/check-packages.sh lite                # comprueba que los paquetes existen en noble
scripts/refresh-rootfs.sh lite                # reaplica la capa RROS sobre un rootfs ya construido
SQUASH_LEVEL=3 scripts/make-iso.sh lite       # ISO más rápida de generar (zstd nivel 3)
scripts/test-iso.sh lite selftest             # arranca en QEMU por serie y ejecuta ~72 comprobaciones
scripts/test-iso.sh lite screenshot           # arranca con GRUB y guarda una captura
python3 scripts/qemu-console.py start|exec|shot|stop   # VM persistente con consola de administrador
```
Sin KVM (por ejemplo en contenedores) QEMU emula por software y el arranque tarda minutos; con KVM es mucho más rápido.

Rootfs de Arch sin ISO (para un contenedor o un chroot; requiere `arch-install-scripts`):
```
sudo scripts/build-rootfs-arch.sh lite
```

## Kernel propio
```
scripts/build-kernel.sh lite /ruta/a/linux-6.x
```
Fusiona `defconfig` + `kernel/base.config` + `kernel/<perfil>.config`. Revisa los avisos de `merge_config.sh`: algunas opciones pueden no existir en tu versión.

## Medir el presupuesto
```
build/rros-budget disk build/rootfs-ubuntu-lite -p lite   # al construir
build/rros-budget ram -p lite                             # dentro del sistema en reposo
```
Sale con código 1 si se supera; pensado para CI.

## Variables de entorno (pruebas)
`RROS_PROFILE_DIR`, `RROS_SYSTEM_CONF`, `RROS_USER_CONF`, `RROS_PROFILE_FILE`, `RROS_ROOT` (raíz falsa para `rros-budget ram`).
