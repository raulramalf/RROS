# RROS

Sistema operativo ligero y muy personalizable. Usa el **kernel Linux** con una base mínima de **Ubuntu** o **Arch Linux**; los componentes propios de RROS están escritos en **C**.

## Objetivos

| Objetivo | Meta |
|---|---|
| Perfiles de hardware | Lite / Standard / Pro (techo de diseño: 8 GB de RAM) |
| Disco | ≤ 20 GB (Lite ≤ 6, Standard ≤ 12, Pro ≤ 20) |
| RAM del SO en reposo | ≤ 4 GB (Lite < 512 MB, Standard < 2 GB, Pro ≤ 4 GB) |
| Arquitecturas | x86_64 (Intel/AMD) y ARM64 |
| Personalización | ~90 ajustes declarativos por capas (ver [docs/CUSTOMIZATION.md](docs/CUSTOMIZATION.md)) |
| Móvil | Android en modelos concretos; iPhone no viable ([docs/MOBILE.md](docs/MOBILE.md)) |

## Estado

| Pieza | Estado |
|---|---|
| `rros-hwdetect` – detecta hardware y elige perfil | ✅ hecho y probado |
| `rros-config` – personalización por capas con validación | ✅ hecho y probado |
| `rros-budget` – comprueba presupuesto de RAM/disco | ✅ hecho y probado |
| `rros-installer plan` – esquema de particiones (simulación) | ✅ hecho y probado; **no escribe en disco todavía** |
| Scripts de rootfs, kernel, ISO y QEMU | ⚠️ escritos, **sin ejecutar** (requieren root, red y herramientas que no había en el entorno de desarrollo) |
| Configuración de kernel por perfil | ⚠️ sin compilar |
| Interfaz gráfica propia, instalador real, repositorio de paquetes, móvil | ⏳ pendiente |

Las medidas de tamaño y RAM de la ISO aún no existen: los presupuestos son objetivos, no resultados.

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
profiles/       lite.toml, standard.toml, pro.toml
kernel/         fragmentos de configuración del kernel por perfil
packages/       listas de paquetes (Ubuntu y Arch) por perfil
scripts/        rootfs, kernel, ISO, QEMU
dist/systemd/   rros-firstboot.service (elige el perfil en el primer arranque)
tests/          unitarios en C + integración (cli.sh)
docs/           arquitectura, perfiles, build, personalización, móvil, hoja de ruta
```
