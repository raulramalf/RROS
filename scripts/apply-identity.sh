#!/usr/bin/env bash
# Escribe la identidad de Lily en un rootfs: os-release, lsb-release, issue.
# uso: apply-identity.sh RAIZ PERFIL [BASE]   (BASE: ubuntu, por defecto, o arch)
# En Arch os-release pertenece al paquete «filesystem»: aquí solo se guarda /etc/rros/identity y lo
# escribe /usr/libexec/rros-arch-setup (hook de pacman) tras instalar o actualizar paquetes.
# shellcheck source=common.sh
. "$(dirname "$0")/common.sh"

root=${1:?uso: $0 RAIZ PERFIL [BASE]}
profile=${2:?uso: $0 RAIZ PERFIL [BASE]}
base=${3:-ubuntu}
check_profile "$profile"
version=$(sed -n 's/^#define RROS_VERSION "\(.*\)"/\1/p' "$ROOT_DIR/src/lib/rros_version.h")
codename=$(sed -n 's/^#define RROS_CODENAME "\(.*\)"/\1/p' "$ROOT_DIR/src/lib/rros_version.h")
if [ -z "$version" ] || [ -z "$codename" ]; then
	die "no se pudo leer la versión"
fi
home=https://github.com/raulramalf/RROS

mkdir -p "${root:?}/etc/rros"
cat > "$root/etc/rros/identity" <<IDN
RROS_VERSION=$version
RROS_CODENAME=$codename
RROS_CODENAME_LC=${codename,,}
RROS_PROFILE=$profile
RROS_HOME=$home
RROS_BASE=$base
IDN
if [ "$base" = ubuntu ]; then
cat > "$root/etc/os-release" <<OSR
PRETTY_NAME="Lily $version ($codename, $profile)"
NAME="Lily"
VERSION_ID="$version"
VERSION="$version ($codename)"
VERSION_CODENAME=${codename,,}
ID=rros
ID_LIKE="ubuntu debian"
HOME_URL="$home"
SUPPORT_URL="$home/issues"
BUG_REPORT_URL="$home/issues"
VARIANT="$profile"
VARIANT_ID=$profile
UBUNTU_CODENAME=noble
OSR
fi
cat > "$root/etc/lsb-release" <<LSB
DISTRIB_ID=Lily
DISTRIB_RELEASE=$version
DISTRIB_CODENAME=${codename,,}
DISTRIB_DESCRIPTION="Lily $version ($codename)"
LSB
printf 'Lily %s (%s) \\n \\l\n\n' "$version" "$codename" > "$root/etc/issue"
printf 'Lily %s (%s)\n' "$version" "$codename" > "$root/etc/issue.net"
echo "lily-$profile" > "$root/etc/hostname"
