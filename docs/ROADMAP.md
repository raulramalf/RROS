# Hoja de ruta

1. **Fase 0 – Cimientos** ✅ repo, docs, `rros-hwdetect`, `rros-config`, `rros-budget`, plan de instalador, tests y CI.
2. **Fase 1 – ISO Lite** ⚠️ scripts escritos; falta ejecutarlos, arrancar en QEMU (BIOS y UEFI) y medir disco/RAM reales.
3. **Fase 2 – Perfiles** primera ISO Standard y Pro; el aplicado de `rros-config` a servicios y compositor (hoy solo guarda y valida los ajustes).
4. **Fase 3 – Instalador real**: ejecutar el plan (`sfdisk`, `mkfs`, GRUB), cifrado LUKS opcional.
5. **Fase 4 – Hardware real**: Intel, AMD, ARM64, drivers, firmware, suspensión.
6. **Fase 5 – Repositorio propio** y actualizaciones atómicas con rollback.
7. **Fase 6 – Interfaz propia** y herramienta gráfica de personalización.
8. **Fase 7 – Android** (modelos soportados).
