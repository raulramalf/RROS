# Probar Lily en una máquina virtual

Hay dos formas: con **Lily ya instalado** (la máquina virtual, lo más rápido) o con la **ISO** (pruebas el sistema en
vivo y el instalador, como en un PC de verdad).

## Qué descargar

En GitHub → pestaña **Actions** → workflow **Arch** → la última ejecución en verde. Abajo, en *Artifacts* (hace
falta haber iniciado sesión en GitHub; cada descarga es un .zip):

| Edición | ISO | Máquina virtual (ya instalado): VirtualBox · VMware y QEMU | RAM para la VM | Disco |
|---|---|---|---|---|
| **Lite** — equipos modestos, sin efectos | `lily-lite-iso` | `lily-lite-virtualbox` · `lily-lite-vmware-qemu` | 2 GB (mejor 3) | 20 GB |
| **Standard** — oficina, correo, música, impresoras | `lily-standard-iso` | `lily-standard-virtualbox` · `lily-standard-vmware-qemu` | 4 GB | 30 GB |
| **Pro** — además vídeo, audio, dibujo, código y contenedores | `lily-pro-iso` | `lily-pro-virtualbox` · `lily-pro-vmware-qemu` | 6 GB (mejor 8) | 40 GB |

La máquina virtual entra con **usuario `lily` y contraseña `lily`**. Cámbiala en cuanto entres: abre la terminal
(Konsole) y escribe `passwd`.

Para todas: **2 procesadores o más** y la aceleración de la CPU activada en la BIOS del PC (VT-x o AMD-V).

## VirtualBox

**Con la máquina virtual (`lily-EDICIÓN-virtualbox`, un disco VDI):**

No uses *Archivo → Importar servicio virtualizado*: eso es para archivos .ova y no reconoce un disco suelto.

1. Descomprime el .zip: queda `lily-EDICIÓN.vdi`. Ponlo en una carpeta donde vaya a quedarse.
2. *Nueva* → Nombre «Lily», Tipo **Linux**, Versión **Arch Linux (64-bit)**. Desmarca «Habilitar EFI».
3. Memoria y procesadores según la tabla.
4. Disco duro: **Usar un archivo de disco duro virtual existente** → el icono de la carpeta → *Añadir* → elige el `.vdi`.
5. Antes de arrancar, en *Configuración → Pantalla*: controlador **VMSVGA**, 128 MB de memoria de vídeo. La
   aceleración 3D puede ir activada o no; si ves fallos gráficos, quítala.
6. Arranca. La ventana se ajusta sola al tamaño (Lily lleva las utilidades de VirtualBox).

**Con la ISO:** igual, pero en el paso 4 «Crear un disco duro virtual ahora» (tamaño de la tabla) y en
*Configuración → Almacenamiento* pon la ISO en la unidad óptica. Arranca: entras en el sistema en vivo; la
bienvenida te ofrece «Instalar Lily» (también en Ajustes de Lily → «Instalar Lily»).

## VMware Workstation / Player

1. *Create a New Virtual Machine* → *I will install the operating system later* → Linux, **Other Linux 6.x kernel
   64-bit**.
2. Memoria y procesadores según la tabla.
3. Con la máquina virtual (`lily-EDICIÓN-vmware-qemu`, un VMDK): al terminar, *Edit virtual machine settings* → quita el disco que se creó → *Add → Hard
   Disk → Use an existing virtual disk* → el `.vmdk` (si pregunta por convertir el formato, di que sí).
   Con la ISO: *CD/DVD → Use ISO image file*.
4. *Options → Advanced → Firmware type*: **BIOS** para la máquina virtual (la ISO arranca con BIOS o UEFI).
5. Arranca. Lily lleva las herramientas de VMware: la pantalla se ajusta y el portapapeles se comparte.

## QEMU, virt-manager y GNOME Boxes (Linux)

QEMU abre el VMDK directamente:

```sh
qemu-system-x86_64 -enable-kvm -m 4096 -smp 2 -cpu host \
  -drive file=lily-standard.vmdk,if=virtio -vga virtio -display gtk,gl=on \
  -device virtio-serial -chardev spicevmc,id=vda,name=vdagent -device virtserialport,chardev=vda,name=com.redhat.spice.0 \
  -nic user,model=virtio-net-pci
```

O con el script del repositorio, que elige la RAM según la edición: `scripts/vm-lily.sh lily-standard.vmdk`
(o con una ISO: `scripts/vm-lily.sh lily-standard.iso`).

virt-manager: *Nueva máquina virtual → Importar imagen de disco existente* → el `.vmdk`, sistema «Arch Linux».
GNOME Boxes prefiere qcow2: `qemu-img convert -O qcow2 lily-standard.vmdk lily-standard.qcow2` y luego
*+ → Crear una máquina virtual → Archivo de imagen*.

## Si algo no va

- **Pantalla negra al arrancar en VirtualBox:** controlador de pantalla VMSVGA y EFI desactivado.
- **Muy lento:** comprueba que la virtualización está activada en la BIOS del PC y dale los procesadores y la RAM
  de la tabla. Lite va bien hasta con 2 GB; Standard y Pro necesitan más.
- **El informe para contarnos qué tal va:** en Lily, abre la terminal y escribe `rros-informe`; luego abre un [informe de hardware](https://github.com/raulramalf/RROS/issues/new?template=hardware.yml)
  en GitHub (indica que es una máquina virtual y cuál).
