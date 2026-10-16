#!/usr/bin/env bash
# Escribe la identidad de RROS en un rootfs: os-release, lsb-release, issue.
# uso: apply-identity.sh RAIZ PERFIL
# shellcheck source=common.sh
. "$(dirname "$0")/common.sh"

root=${1:?uso: $0 RAIZ PERFIL}
profile=${2:?uso: $0 RAIZ PERFIL}
check_profile "$profile"
version=$(sed -n 's/^#define RROS_VERSION "\(.*\)"/\1/p' "$ROOT_DIR/src/lib/rros_version.h")
codename=$(sed -n 's/^#define RROS_CODENAME "\(.*\)"/\1/p' "$ROOT_DIR/src/lib/rros_version.h")
[ -n "$version" ] && [ -n "$codename" ] || die "no se pudo leer la versión"
home=https://github.com/raulramalf/RROS

mkdir -p "${root:?}/etc"
cat > "$root/etc/os-release" <<OSR
PRETTY_NAME="RROS $version ($codename, $profile)"
NAME="RROS"
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
cat > "$root/etc/lsb-release" <<LSB
DISTRIB_ID=RROS
DISTRIB_RELEASE=$version
DISTRIB_CODENAME=${codename,,}
DISTRIB_DESCRIPTION="RROS $version ($codename)"
LSB
printf 'RROS %s (%s) \\n \\l\n\n' "$version" "$codename" > "$root/etc/issue"
printf 'RROS %s (%s)\n' "$version" "$codename" > "$root/etc/issue.net"
echo "rros-$profile" > "$root/etc/hostname"
