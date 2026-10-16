# RROS

Sistema operativo ligero y muy personalizable. Usa el **kernel Linux** con una base mínima de **Ubuntu** o **Arch Linux**; los componentes propios de RROS están escritos en **C**.

## Objetivos

| Objetivo | Meta |
|---|---|
| Perfiles de hardware | Lite / Standard / Pro (techo de diseño: 8 GB de RAM) |
| Disco | ≤ 20 GB (Lite ≤ 6, Standard ≤ 12, Pro ≤ 20) |
| RAM del SO en reposo | ≤ 4 GB (Lite < 512 MB, Standard < 2 GB, Pro ≤ 4 GB) |
| Arquitecturas | x86_64 (Intel/AMD) y ARM64 |
| Personalización | 89 ajustes declarativos por capas (escritorio, red, memoria y servicios ya se aplican; el resto, validados) (ver [docs/CUSTOMIZATION.md](docs/CUSTOMIZATION.md)) |
| Móvil | Android en modelos concretos; iPhone no viable ([docs/MOBILE.md](docs/MOBILE.md)) |

## Probarlo
- **Navegador:** demo interactiva en [`preview/index.html`](preview/index.html).
- **Contenedor:** `scripts/try-docker.sh lite` (identidad y herramientas).
- **Máquina virtual:** ISO construida en GitHub Actions (workflow *ISO*). Ver [docs/TRY.md](docs/TRY.md).

## Estado

| Pieza | Estado |
|---|---|
| `rros-hwdetect`, `rros-config`, `rros-budget`, `rros-installer plan` | ✅ hechas y probadas |
| `rros-theme` – genera sway, waybar y foot desde los ajustes | ✅ hecha y probada |
| `rros-apply` – aplica perfil a zram, swap, disco, red y servicios | ✅ hecha y probada (con raíz alternativa) |
| `rros-fetch`, `rros` (`doctor`, `info`, …) | ✅ hechas y probadas |
| Overlay de identidad (sin snap, sin telemetría, tty1 directo) | ✅ aplicado y comprobado sobre un directorio |
| ISO: rootfs, GRUB, autologin live, QEMU | ⚠️ escrita, **sin ejecutar** |
| Kernel por perfil, SwayFX, instalador real, repositorio propio, móvil | ⏳ pendiente |

Las medidas de tamaño y RAM de la ISO aún no existen: los presupuestos son objetivos, no resultados. Diferencias con Ubuntu: [docs/DIFFERENCES.md](docs/DIFFERENCES.md).

## Uso rápido

```
make                 # compila las herramientas (build/)
make test            # tests unitarios (ASan+UBSan) e integración
build/rros-hwdetect  # hardware detectado y perfil recomendado
build/rros-installer plan --disk-gb 64 --uefi -p standard
```

Personalizar (por usuario o `--system`):

```
RROS_PROFILE_DIR=profiles build/rros-config -p pro set ui.accent '#ff8800'
RROS_PROFILE_DIR=profiles build/rros-config -p pro list
```

Construir una ISO (en Ubuntu, como root; ver [docs/BUILD.md](docs/BUILD.md)).

## Estructura

```
src/lib/        conf (parser TOML), hw (detección), perfiles, esquema
src/hwdetect/   rros-hwdetect        src/config/   rros-config
src/budget/     rros-budget          src/installer/ rros-installer
src/theme/      rros-theme           src/apply/    rros-apply
src/fetch/      rros-fetch           src/rros/     rros
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
