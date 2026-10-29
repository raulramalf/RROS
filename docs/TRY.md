# Probar Lily

## 1. En una máquina virtual (VirtualBox, QEMU, GNOME Boxes)
Descarga la ISO de la última ejecución en verde del workflow **Arch** (pestaña *Actions* → *Artifacts* →
`lily-iso`, un .zip con la ISO; hay que haber iniciado sesión en GitHub). Es una sola versión: se ajusta sola a la
RAM y los procesadores de cada equipo.

**VirtualBox**
- Tipo *Linux*, versión *Arch Linux (64-bit)*; 4 GB de RAM (mínimo 2) y 2 procesadores o más; disco de 20 GB.
- *Pantalla*: controlador **VMSVGA**, 128 MB de vídeo y **Habilitar aceleración 3D**. Sin 3D Lily va igual de
  bien, pero apaga los efectos.
- EFI es opcional (funciona con y sin él).
- Lily trae las utilidades de VirtualBox: la pantalla se ajusta al tamaño de la ventana.
- Si al arrancar sale *AMD-V is disabled* o *VT-x is disabled*: hay que activar la virtualización en la BIOS del
  PC (opción *SVM Mode* en AMD, *Intel Virtualization Technology* en Intel). Si Windows la tiene ocupada,
  desactiva *Integridad de memoria* (Seguridad de Windows → Aislamiento del núcleo) y la característica Hyper-V.

**QEMU**
```
scripts/run-qemu.sh lite            # BIOS
scripts/run-qemu.sh lite --uefi     # UEFI (requiere OVMF)
```

El sistema arranca solo en el escritorio con el usuario `rros` (sin contraseña) y abre la bienvenida. Desde ahí,
o desde el menú Lily → *Instalar Lily…*, se instala en el disco.

## 2. Instalarlo en un disco
Asistente gráfico en Ajustes de Lily → *Instalar Lily*: idioma, disco (junto a Windows en el espacio libre o en
todo el disco), cifrado opcional, usuario y resumen. También en la terminal:
```
rros install --disk /dev/vda --dry-run              # solo muestra lo que haría
rros install                                         # asistente de texto
```
La CI lo prueba de principio a fin en cada ISO: instala en un disco virtual (BIOS, UEFI y cifrado con LUKS2),
arranca desde él y pasa el autotest.

## 3. En un PC real
[PROBAR-EN-PC.md](PROBAR-EN-PC.md): memoria USB, Secure Boot desactivado, y cómo enviar un informe del hardware.

## 4. En el navegador
`site/escritorio.html` (y su publicación en Vercel) es una demostración del aspecto, no el sistema.

## Qué mirar al arrancar
- El menú Lily (arriba a la izquierda): este equipo, ajustes, tienda, apagar.
- El dock: Launchpad (todas las aplicaciones), Archivos, Firefox, Tienda, Terminal, Kate y Ajustes.
- Ajustes de Lily → Escritorio y Dock: el dock en 2D o 3D y los estilos Moderno, Clásico y Enfoque.
- `rros-fetch` y `rros doctor` en la terminal. La guía de comandos está en [CONSOLA.md](CONSOLA.md).
