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
| **rros-hwdetect** | **C, propio.** Detecta RAM/CPU/GPU/UEFI y elige perfil (CPU y GPU solo pueden bajarlo) |
| **rros-config** | **C, propio.** Capas perfil < sistema < usuario, validadas con un esquema |
| **rros-budget** | **C, propio.** Mide disco/RAM frente al presupuesto del perfil (CI) |
| **rros-theme** | **C, propio.** 7 temas más los del usuario; genera sway, waybar, foot, mako, fuzzel y GTK |
| **rros-settings** | **C, propio.** Interfaz de ajustes: servidor web local con token (GUI en `gui/`) y TUI |
| **rros-installer** | **C, propio.** Hoy solo calcula el plan de particiones (no escribe en disco) |

## Presupuesto de recursos
- Disco: imagen base ≤ 6 GB, perfil Pro completo ≤ 20 GB.
- RAM SO en reposo: Lite < 512 MB, Standard < 2 GB, Pro ≤ 4 GB.
- CI mide ambos en cada build y falla si se superan.

## Personalización "al detalle"
Todo ajuste vive en archivos TOML: `/usr/share/rros/profiles/<perfil>.toml` (valores por defecto) < `/etc/rros/rros.toml` (sistema) < `~/.config/rros/rros.toml` (usuario). `rros-config` valida cada valor (tipo, rango, opciones). Las claves disponibles están en [CUSTOMIZATION.md](CUSTOMIZATION.md). Aplicar los ajustes a compositor y servicios llegará con la interfaz propia.

## Compatibilidad de hardware
Soporte amplio mediante el kernel Linux + firmware empaquetado (`linux-firmware`). Intel, AMD y ARM64 desde el inicio. NVIDIA: driver abierto `nouveau`/`nvidia-open` opcional.
