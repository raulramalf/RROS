# Ediciones de Lily (perfiles de hardware)

Hay tres ediciones de la misma versión. Cambian los efectos, el dock, la memoria (zram) y las aplicaciones que
vienen instaladas; el resto es igual. Todas usan como mucho **20 GB de disco** y están pensadas para equipos de
hasta **8 GB de RAM**.

| | Lite | Standard | Pro |
|---|---|---|---|
| RAM del equipo | 2–4 GB (mínimo 2 GB) | 4–8 GB (mínimo 4 GB) | 8 GB (mínimo 6 GB) |
| Procesador | 2 núcleos x86-64 (cualquiera de los últimos ~12 años) | 4 hilos | 4 núcleos / 8 hilos |
| Gráfica | cualquiera, también sin aceleración | integrada | integrada moderna o dedicada |
| Disco (tope de la instalación) | 10 GB | 14 GB | 20 GB |
| RAM del sistema en reposo (tope) | 1,3 GB | 2 GB | 3 GB |
| Efectos | ninguno | básicos (animaciones rápidas) | todos (desenfoque, lámpara mágica, transparencias) |
| Dock | 2D, iconos de 44 px | 2D, 48 px | 3D, 52 px |
| Escritorios virtuales | 1 | 2 | 4 |
| zram | 50 % de la RAM, lz4 | 25 %, zstd | 25 %, zstd |
| Aplicaciones de más | — | LibreOffice, Elisa, impresoras, escáner, móviles | + GIMP, VLC, particiones, KDE Connect, desarrollo |

**Procesador en reposo:** el autotest mide cuánto procesador gasta el sistema sin hacer nada (media de 10 s) y lo
escribe en el registro de la CI (`info: procesador en reposo`). Con el escritorio quieto debe quedar en unos
pocos puntos porcentuales; los números de cada edición están en el README cuando se miden.

Sin aceleración gráfica (máquina virtual sin 3D o «modo seguro» del menú de arranque) KWin dibuja con el
procesador; Lily lo detecta al iniciar sesión y apaga los efectos aunque la edición los tenga.

## Selección automática
`rros-hwdetect` lee `/proc/meminfo` y `/proc/cpuinfo` y propone la edición (Ajustes de Lily → General lo muestra).
El usuario siempre puede cambiar cualquier ajuste.
