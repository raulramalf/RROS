# Equipos comprobados con Lily

Cada fila sale de un informe real (`rros informe`) y de las pruebas a mano de la plantilla de GitHub
(«Informe de hardware»). El informe ya da un **veredicto automático** de Wi-Fi, suspensión, sonido, gráfica NVIDIA,
aceleración 3D, Bluetooth, cámara, panel táctil, batería y Secure Boot.

| Equipo | Edición | Wi-Fi | Sonido | Suspensión | Gráfica | Notas |
|---|---|---|---|---|---|---|
| QEMU/KVM (CI, cada versión) | Lite | — | — | — | llvmpipe | arranque, instalación BIOS/UEFI/cifrada y autotest |
| VirtualBox 7 (VMSVGA, 3D) | Lite | — | ✅ | — | ✅ | sin AMD-V/VT-x en la BIOS no arranca (docs/TRY.md) |

Todavía **no hay portátiles reales** en la lista: hace falta probar Lily en hardware de verdad.

## Cómo añadir tu equipo

1. Arranca el live o el sistema instalado y ejecuta `rros informe`. Se guarda en `~/rros-informe-FECHA.md`, sin
   números de serie ni direcciones MAC.
2. Prueba a mano lo que el informe no puede: el Wi-Fi conectándote a una red, el sonido con un vídeo, la suspensión
   cerrando la tapa y la vuelta, el brillo y las teclas de función.
3. Abre un *issue* «Informe de hardware» en GitHub con el informe y lo que probaste.

Cuando un modelo llegue a tres informes buenos, entra en la tabla.
