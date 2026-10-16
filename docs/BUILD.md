# Construir RROS

## Herramientas en C (cualquier Linux)
```
make WERROR=1 && make test
```
Necesita `gcc` y `make`.

## ISO sobre Ubuntu 24.04 (estado: sin probar)
Requisitos (Ubuntu/Debian): `debootstrap squashfs-tools grub-pc-bin grub-efi-amd64-bin xorriso mtools qemu-system-x86 ovmf`.
```
make
sudo scripts/build-rootfs-ubuntu.sh lite      # rootfs + presupuesto de disco
sudo scripts/make-iso.sh lite                 # build/rros-lite.iso
scripts/run-qemu.sh lite [--uefi]             # RAM acotada al perfil
```

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
