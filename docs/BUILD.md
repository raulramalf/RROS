# Construir RROS

## Herramientas en C (cualquier Linux)
```
make WERROR=1 && make test
```
Necesita `gcc` y `make`.

## ISO sobre Ubuntu 24.04 (estado: Lite construida y arrancada en QEMU; Standard y Pro sin construir)
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
scripts/test-iso.sh lite selftest             # arranca en QEMU por serie y ejecuta ~59 comprobaciones
scripts/test-iso.sh lite screenshot           # arranca con GRUB y guarda una captura
python3 scripts/qemu-console.py start|exec|shot|stop   # VM persistente con consola de administrador
```
Sin KVM (por ejemplo en contenedores) QEMU emula por software y el arranque tarda minutos; con KVM es mucho más rápido.

## Rootfs sobre Arch (estado: sin probar)
Requiere `arch-install-scripts` en un Arch.
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
