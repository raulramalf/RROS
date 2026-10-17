# Seguridad en RROS

RROS trae directivas de seguridad activadas desde el primer arranque y una herramienta, `rros seguridad` (`rros-security`), que comprueba el estado real del equipo y dice cómo arreglar cada punto. También está en **RROS Ajustes → Seguridad**.

## Qué está activado por defecto (nivel «estándar»)

| Directiva | Clave | Qué hace | Archivo que genera `rros-apply` |
|---|---|---|---|
| Cortafuegos | `security.firewall` | Bloquea las conexiones entrantes que no has pedido. Las respuestas a lo que tú abres (web, actualizaciones) entran sin problema | `/etc/nftables.conf` (solo la tabla `inet rros`; no toca reglas de Docker o libvirt) |
| Ping | `security.firewall_ping` | Responder a ping (limitado a 10 por segundo) | — |
| Puertos abiertos | `security.firewall_allow` | Lista de puertos que sí dejan entrar, p. ej. `8080/tcp,5000-5010/udp` | — |
| Kernel endurecido | `security.kernel_hardening` | Oculta direcciones del kernel, restringe `dmesg`, BPF y `ptrace`, ignora redirecciones ICMP, protege enlaces y archivos en carpetas compartidas | `/etc/sysctl.d/80-rros-security.conf` |
| Intentos de contraseña | `security.login_attempts` | Tras N fallos (10 por defecto) la cuenta se bloquea 10 minutos (no afecta a root) | `/etc/security/faillock.conf` (en Arch, `pam_faillock` ya va en `system-auth`; en Ubuntu se activan los perfiles PAM `rros-faillock*`) |
| sudo | `security.sudo_timeout_minutes` | Minutos que `sudo` recuerda tu contraseña (5; 0 = la pide siempre) | `/etc/sudoers.d/rros` (comprobado con `visudo` antes de instalarlo) |
| Volcados de memoria | `security.core_dumps` | Sin volcados (pueden contener contraseñas) | `/etc/security/limits.d/90-rros-core.conf` |
| Memorias USB | `security.usb_storage` | Permitidas | `/etc/modprobe.d/rros-security.conf` |
| Cámara | `security.camera` | Permitida | `/etc/modprobe.d/rros-security.conf` |
| Actualizaciones | `system.auto_updates` | Arch: se **descargan** solas cada día (`checkupdates --download`, sin tocar la base de datos del sistema) y se instalan cuando quieras con `rros apps update` o el botón «ACT» del panel. Ubuntu: las de seguridad se instalan solas | Arch: `rros-updates.timer`. Ubuntu: `/etc/apt/apt.conf.d/20auto-upgrades` |
| AppArmor | — | Confinamiento de programas. En Arch se activa con `lsm=…,apparmor,…` en el arranque (ISO e instalado) y carga los perfiles del paquete `apparmor`; en Ubuntu, los suyos | — |

Los servicios que activas abren su puerto solos: `services.ssh` abre el 22/tcp y `services.printing` el mDNS para encontrar impresoras.

## Niveles

```sh
rros seguridad niveles
sudo rros seguridad nivel estricto
```

| Nivel | Para quién |
|---|---|
| `basico` | Como Ubuntu: solo cortafuegos |
| `estandar` | Recomendado (el de fábrica) |
| `estricto` | Equipos que salen de casa o con datos delicados: sin memorias USB ni cámara, sin ping ni SSH, bloqueo tras 5 intentos y `sudo` pide la contraseña siempre |

Un nivel solo escribe un grupo de claves en `/etc/rros/rros.toml` y aplica; después puedes cambiar cualquiera por separado.

## Auditoría

```sh
rros seguridad                 # en texto, con colores
rros seguridad auditoria --json
```

Comprueba, en el equipo real: cortafuegos activo, programas escuchando en la red (y si el cortafuegos los protege), ajustes del kernel aplicados, AppArmor, actualizaciones automáticas, bloqueo por intentos, `sudo`, bloqueo de pantalla, cuentas con UID 0 o sin contraseña (esto último solo con `sudo`), servidor SSH, cifrado del disco, Secure Boot y volcados de memoria. La puntuación es el porcentaje de comprobaciones correctas (las informativas no cuentan).

## Cambiar una directiva a mano

Son ajustes del sistema: necesitan `sudo` y `--system`, y se aplican con `rros apply --services`.

```sh
sudo rros config set --system security.usb_storage false
sudo rros apply --services
sudo rros seguridad abrir 25565/tcp      # atajo para security.firewall_allow
```

En RROS Ajustes, estos ajustes llevan la marca «sistema · contraseña»: al cambiarlos se abre una terminal que pide tu contraseña y aplica.

## Límites conocidos

- Bloquear memorias USB o la cámara no expulsa lo que ya está conectado si está en uso: vale para lo que se conecte después o tras reiniciar.
- Desactivar el endurecimiento del kernel no deshace los valores ya aplicados hasta reiniciar.
- En Arch las actualizaciones no se instalan solas: es una decisión, porque en una distribución rolling una actualización puede pedir que mires algo antes. El panel avisa cuando hay.
- (Base Ubuntu) En el sistema live Ubuntu no carga los perfiles de AppArmor; como la restricción de espacios de nombres de Ubuntu dejaría sin arrancar el navegador y las apps Flatpak, RROS la desactiva **solo en el live** (`rros-live-userns.service`). En el sistema instalado la restricción sigue activa y los perfiles `epiphany` y `flatpak` dan el permiso.

## Qué se ha comprobado

En Arch: las pruebas automáticas cubren lo que `rros-apply` escribe (faillock, `locale.conf`, `sshd.service`, sin archivos de Ubuntu) y lo que comprueba la auditoría (`system-auth`, el timer de actualizaciones). En la CI, el autotest comprueba en el live y en el sistema ya instalado (también cifrado) el cortafuegos, el kernel endurecido, AppArmor activo con perfiles cargados, faillock, sudo, el timer de actualizaciones y la auditoría.

En QEMU, sobre la ISO Lite de la base Ubuntu: cortafuegos cargado (`nft list table inet rros`), ajustes del kernel aplicados, `sudoers` válido, actualizaciones automáticas, la auditoría real (100/100 en el live con el nivel estándar) y el bloqueo por intentos: con `security.login_attempts = 3`, tres contraseñas malas con `su` bloquean la cuenta, la buena se rechaza («10 minutes left to unlock») y tras `faillock --reset` vuelve a entrar. También que Epiphany arranca con la restricción de espacios de nombres y el perfil cargado (como en el sistema instalado). Falta probarlo en hardware real y repetir una instalación completa con estas directivas.
