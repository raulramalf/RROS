#!/usr/bin/env bash
# Compila un kernel con la configuración de RROS: defconfig + base + <perfil>.
# uso: build-kernel.sh PERFIL DIR_FUENTES_KERNEL
# shellcheck source=common.sh
. "$(dirname "$0")/common.sh"

profile=${1:?uso: $0 PERFIL DIR_KERNEL}
src=${2:?uso: $0 PERFIL DIR_KERNEL}
check_profile "$profile"
[ -f "$src/Makefile" ] || die "$src no parece un árbol del kernel"

cd "$src"
make defconfig
scripts/kconfig/merge_config.sh -m .config \
	"$ROOT_DIR/kernel/base.config" "$ROOT_DIR/kernel/$profile.config"
make olddefconfig
make -j"$(nproc)"
log "kernel listo: $src/arch/x86/boot/bzImage"
