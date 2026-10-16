# Probar RROS

Hay tres formas, de la más rápida a la más real.

## 1. En el navegador (sin instalar nada)
La demo interactiva está en [`preview/index.html`](../preview/index.html). Ábrela con doble clic o sírvela con `python3 -m http.server -d preview`.
Simula el escritorio: perfiles, tema, panel, acento y una terminal con `rros-fetch`, `rros-config`, `rros doctor` y `rros-installer plan`. Los valores de memoria y disco son de ejemplo.

## 2. En un contenedor (identidad y herramientas reales)
```
scripts/try-docker.sh lite     # o standard, pro
```
Abre una consola con la identidad de RROS, el prompt, el banner (`rros-fetch`), `rros doctor` y todas las herramientas en C compiladas. No arranca escritorio ni kernel.

## 3. ISO en una máquina virtual (el sistema completo)
Estado: los scripts están escritos pero **no se han ejecutado todavía**. El primer arranque probablemente necesite ajustes.

**Sin tocar tu PC:** en GitHub, pestaña *Actions*, workflow **ISO**, *Run workflow*, elige el perfil. Al terminar, descarga el artefacto `rros-<perfil>-iso`.

**En tu PC (Ubuntu):** ver [BUILD.md](BUILD.md).

Arranque en QEMU:
```
scripts/run-qemu.sh lite            # BIOS
scripts/run-qemu.sh lite --uefi     # UEFI (requiere OVMF)
```
En VirtualBox o GNOME Boxes: tipo Linux/Ubuntu 64 bits, la RAM del perfil (Lite 1 GB, Standard 4 GB, Pro 8 GB), activa EFI si quieres y monta la ISO. El sistema live entra solo con el usuario `rros` en tty1 y arranca el escritorio.

## Qué mirar al arrancar
- `rros-fetch` y `rros doctor`: identidad, perfil y presupuesto.
- `Mod+T` alterna tema oscuro/claro; `Mod+P` abre la ficha; `Mod+D` el lanzador.
- `rros config set ui.accent '#22d3ee'` y `rros theme apply` cambian el escritorio.
