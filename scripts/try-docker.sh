#!/usr/bin/env bash
# Abre una consola de RROS en un contenedor. uso: try-docker.sh [PERFIL]
# shellcheck source=common.sh
. "$(dirname "$0")/common.sh"

profile=${1:-standard}
check_profile "$profile"
need_cmd docker
docker build --build-arg PROFILE="$profile" -t "rros:$profile" "$ROOT_DIR"
exec docker run --rm -it "rros:$profile"
