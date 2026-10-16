# Hoja de ruta

1. **Fase 0 – Cimientos**: repo, docs, `rros-hwdetect` (C), CI con medición de tamaño.
2. **Fase 1 – ISO Lite**: base mínima + kernel recortado, arranca en QEMU/UEFI.
3. **Fase 2 – Perfiles**: Standard y Pro, selección automática, `rros-config`.
4. **Fase 3 – Instalador** (`rros-installer`): UEFI/BIOS, cifrado LUKS opcional.
5. **Fase 4 – Hardware real**: Intel/AMD/ARM64, drivers, firmware, suspensión.
6. **Fase 5 – Repositorio propio** y actualizaciones atómicas con rollback.
7. **Fase 6 – Mobile (Android)**.
