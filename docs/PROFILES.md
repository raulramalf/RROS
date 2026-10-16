# Perfiles de hardware

| Perfil | RAM equipo | CPU/GPU | RAM SO en reposo | Disco | Escritorio |
|---|---|---|---|---|---|
| **Lite** | 1–2 GB | Antigua / sin GPU dedicada | < 512 MB | ≤ 6 GB | Compositor Wayland mínimo, sin efectos |
| **Standard** | 4 GB | Gama media | < 2 GB | ≤ 12 GB | Compositor completo, animaciones suaves |
| **Pro** | 8 GB | Gama alta | ≤ 4 GB | ≤ 20 GB | Efectos, aceleración GPU, herramientas avanzadas |

El techo de diseño es **8 GB de RAM** en el equipo. Más memoria se aprovecha para cachés, pero no se requiere.

## Selección automática
`rros-hwdetect` lee `/proc/meminfo` y `/proc/cpuinfo`, y propone el perfil. El usuario siempre puede forzarlo.
