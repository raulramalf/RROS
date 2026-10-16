# Arquitectura

## Decisión clave: "desde cero en C" vs. "basado en Ubuntu/Arch"
Escribir un kernel propio supone años de trabajo en drivers (Intel, AMD, NVIDIA, Wi-Fi...). RROS usa el **kernel Linux** (ya en C) y escribe en C todo lo que le da identidad:

| Capa | Origen |
|---|---|
| Kernel | Linux LTS, configuración recortada por perfil |
| Base | Mínima, estilo Arch (rolling) con empaquetado/pruebas estilo Ubuntu (LTS) |
| Init | Opción A: systemd recortado. Opción B: init propio en C (fase avanzada) |
| libc | glibc (compatibilidad) en Standard/Pro; musl en Lite |
| Gráficos | Wayland. Lite: compositor mínimo; Pro: efectos y aceleración |
| **rros-hwdetect** | **C, propio.** Detecta CPU/RAM/GPU y elige perfil |
| **rros-config** | **C, propio.** Motor de personalización declarativa |
| **rros-installer** | **C, propio.** Instalador por perfil |

## Presupuesto de recursos
- Disco: imagen base ≤ 6 GB, perfil Pro completo ≤ 20 GB.
- RAM SO en reposo: Lite < 512 MB, Standard < 2 GB, Pro ≤ 4 GB.
- CI mide ambos en cada build y falla si se superan.

## Personalización "al detalle"
Todo ajuste vive en archivos declarativos (`~/.config/rros/*.toml`) con valores por defecto del perfil. `rros-config` valida, aplica y revierte cambios.

## Compatibilidad de hardware
Soporte amplio mediante el kernel Linux + firmware empaquetado (`linux-firmware`). Intel, AMD y ARM64 desde el inicio. NVIDIA: driver abierto `nouveau`/`nvidia-open` opcional.
