# Probar RROS en un PC real

RROS solo se ha probado en máquinas virtuales (QEMU). Esta guía es para dar el paso a un equipo de verdad
sin riesgo y para contar qué funciona, de forma que cada informe sirva para arreglar algo.

**Sin riesgo:** el sistema live (arrancar desde la memoria USB) no toca el disco. Solo el instalador escribe en
él, y únicamente cuando lo confirmas.

## 1. Lo que necesitas

- La ISO (`rros-arch-lite.iso`). Se descarga de los artefactos del workflow **Arch** de GitHub Actions o se
  genera con [BUILD.md](BUILD.md). (La ISO de la base Ubuntu, `rros-lite.iso`, sigue sirviendo y es la única que
  arranca con Secure Boot.)
- Una memoria USB de 4 GB o más (se borra entera).
- Para instalar: 10 GB libres (Lite). Si vas a instalarlo junto a Windows, una copia de seguridad y, si Windows
  usa BitLocker, su clave de recuperación (en tu cuenta Microsoft → Dispositivos).

## 2. Grabar la memoria USB

La ISO es «híbrida» (como la de Ubuntu): se copia tal cual a la memoria, sin descomprimir ni copiar archivos.

| Desde | Cómo |
|---|---|
| Linux | `sudo dd if=rros-lite.iso of=/dev/sdX bs=4M status=progress conv=fsync` (sdX es la memoria: compruébalo con `lsblk`), o «Discos» → *Restaurar imagen* |
| Windows | [Rufus](https://rufus.ie): elige la ISO y, cuando pregunte, **modo imagen DD** |
| macOS / cualquiera | [balenaEtcher](https://etcher.balena.io) |
| Ventoy | Copia la ISO a la memoria de Ventoy (funciona, pero si algo falla prueba con DD para descartar Ventoy) |

## 3. Arrancar desde la memoria

1. Con el equipo apagado, conecta la memoria (mejor en un puerto USB de la placa, no en un hub).
2. Enciende y pulsa la tecla del **menú de arranque**:

   | Marca | Tecla | | Marca | Tecla |
   |---|---|---|---|---|
   | Dell | F12 | | Acer | F12 (actívalo en la BIOS si no sale) |
   | HP | F9 (o Esc y luego F9) | | ASUS | F8 o Esc |
   | Lenovo | F12 o Fn+F12 (ThinkPad: Enter → F12) | | MSI | F11 |
   | Gigabyte | F12 | | Mac Intel | mantén Opción (⌥) |

3. Elige la entrada con el nombre de la memoria (en UEFI suele empezar por «UEFI:»).
4. Sale el menú de RROS. La primera opción arranca el sistema live; espera al escritorio (en un USB 2.0 puede
   tardar un par de minutos).

**Secure Boot: desactívalo** en la BIOS/UEFI (pestaña *Security* o *Boot*, opción *Secure Boot* → *Disabled*)
antes de arrancar la ISO de Arch: Arch no firma su arranque y, con Secure Boot activo, el equipo la rechaza
(«Security Violation» o vuelve al menú). Windows sigue arrancando igual con Secure Boot desactivado; si usa
BitLocker, ten a mano la clave de recuperación por si la pide al cambiarlo.

**Pantalla negra o congelada:** reinicia y elige **«modo seguro (gráficos básicos)»** en el menú. Arranca sin el
controlador de la tarjeta gráfica (resolución fija y sin aceleración); dilo en el informe junto con la tarjeta.

**Con lector de pantalla:** la entrada «con lector de pantalla» del menú arranca con Orca activado desde el
principio (voz en español).

**Windows con «Inicio rápido»:** si vas a instalar junto a Windows, desactívalo antes (Panel de control →
Opciones de energía → *Elegir el comportamiento de los botones* → quita «Activar inicio rápido»). Si no, Windows
deja su disco «a medias» y RROS no debe tocarlo.

## 4. Qué probar en el live

- **Red:** el icono del panel (Wi-Fi) y el cable.
- **Sonido:** el control de volumen del panel; un vídeo en el navegador. También auriculares.
- **Pantalla:** resolución, brillo (teclas de función), pantalla externa por HDMI.
- **Teclado y touchpad:** la ñ, las tildes, el clic con dos dedos y el desplazamiento.
- **Bluetooth, cámara y memorias USB:** RROS Ajustes → *Dispositivos* lo enseña todo.
- **Suspender:** cierra la tapa o Super + Mayús + E → Suspender, y vuelve.
- **Batería:** el porcentaje en el panel.

## 5. Instalar

Desde el live: RROS Ajustes → **Instalar RROS** (o la bienvenida → *Instalar RROS*). Cuatro pasos:

1. **Idioma, teclado y zona horaria.**
2. **Disco.** Para cada disco dice qué tiene (Windows, Linux) y cuánto espacio libre queda:
   - **Junto a Windows (en el espacio libre):** no borra nada; crea las particiones de RROS en el hueco libre y
     reutiliza la partición EFI de Windows. Al encender, el menú deja elegir RROS o *Windows Boot Manager*.
     El hueco hay que dejarlo antes **desde Windows**: Administración de discos → clic derecho en (C:) →
     *Reducir volumen* (10 GB o más). Solo en discos GPT (los de cualquier PC con Windows 10/11 en UEFI).
   - **Borrar todo el disco.** Pide marcar una casilla de confirmación.
   - **Cifrar el disco** (opcional): LUKS2, como Ubuntu. Al encender pide la contraseña, con tu distribución de
     teclado. Si la olvidas, los datos no se pueden recuperar.
3. **Tu cuenta:** nombre, usuario, contraseña y nombre del equipo.
4. **Resumen** e *Instalar ahora*. La barra muestra el progreso (la copia es lo más largo); al terminar,
   *Reiniciar ahora* y quita la memoria cuando se apague la pantalla.

En la consola es lo mismo: `rros install` (asistente de texto) o, sin preguntas,
`rros install --disk /dev/nvme0n1 --alongside --encrypt --user ana --password-file F --passphrase-file G --yes`.

**Si después no aparece el menú para elegir Windows:** algunos firmwares arrancan siempre «Windows Boot
Manager». Entra en el menú de arranque (paso 3) y elige «RROS», o pon RROS primero en el orden de arranque de la
BIOS/UEFI.

## 6. Contarlo

1. En RROS (live o instalado), abre una terminal (Super + Intro) y ejecuta:
   ```sh
   rros informe
   ```
   Muestra el equipo, el firmware, Secure Boot, las tarjetas y su controlador (las que dicen
   **SIN CONTROLADOR** son las candidatas a no funcionar), los dispositivos, la red, el sonido y los errores del
   arranque. Lo guarda en `~/rros-informe-FECHA.md`. No incluye números de serie, direcciones MAC ni nombres.
2. Abre un [informe de hardware](https://github.com/raulramalf/RROS/issues/new?template=hardware.yml) en
   GitHub, marca lo que funciona y pega el informe. Sirve aunque todo vaya bien: saber qué equipos funcionan
   también es útil.

## Qué se ha comprobado (en QEMU)

Se actualiza con cada prueba; en hardware real, de momento, nada.

| Prueba | Resultado |
|---|---|
| Live por BIOS (kernel directo) | ✅ autotest completo |
| Instalación con BIOS, borrando el disco | ✅ arranca, entra en el escritorio |
