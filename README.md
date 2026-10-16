# RROS

Sistema operativo ligero y altamente personalizable, basado en el **kernel Linux** con una base mínima derivada de **Ubuntu** y **Arch Linux**. Los componentes propios de RROS se escriben en **C**.

## Objetivos

| Objetivo | Meta |
|---|---|
| Perfiles de hardware | Lite / Standard / Pro (hasta 8 GB de RAM) |
| Tamaño en disco | **≤ 20 GB** (instalación completa) |
| RAM consumida por el SO | **2–4 GB** como máximo (Lite: < 512 MB en reposo) |
| Arquitecturas | x86_64 (Intel/AMD), ARM64; RISC-V a futuro |
| Personalización | Temas, atajos, comportamiento, todo declarativo |
| Móvil | Android (modelos soportados); iPhone no viable (ver `docs/MOBILE.md`) |

## Estructura

```
docs/        Arquitectura, perfiles, móvil, hoja de ruta
profiles/    Definición declarativa de cada perfil
src/hwdetect Detector de hardware que elige el perfil (C)
```

## Compilar el detector

```
make
./build/rros-hwdetect
```

Ver [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) y [docs/ROADMAP.md](docs/ROADMAP.md).
