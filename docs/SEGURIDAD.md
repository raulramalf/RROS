# Seguridad en RROS

RROS trae directivas de seguridad activadas desde el primer arranque y una herramienta, `rros seguridad` (`rros-security`), que comprueba el estado real del equipo y dice cómo arreglar cada punto. También está en **RROS Ajustes → Seguridad**.

## Qué está activado por defecto (nivel «estándar»)

| Directiva | Clave | Qué hace | Archivo que genera `rros-apply` |
|---|---|---|---|
| Cortafuegos | `security.firewall` | Bloquea las conexiones entrantes que no has pedido. Las respuestas a lo que tú abres (web, actualizaciones) entran sin problema | `/etc/nftables.conf` (solo la tabla `inet rros`; no toca reglas de Docker o libvirt) |
| Ping | `security.firewall_ping` | Responder a ping (limitado a 10 por segundo) | — |
| Puertos abiertos | `security.firewall_allow` | Lista de puertos que sí dejan entrar, p. ej. `8080/tcp,5000-5010/udp` | — |
| Kernel endurecido | `security.kernel_hardening` | Oculta direcciones del kernel, restringe `dmesg`, BPF y `ptrace`, ignora redirecciones ICMP, protege enlaces y archivos en carpetas compartidas | `/etc/sysctl.d/80-rros-security.conf` |
| Intentos de contraseña | `security.login_attempts` | Tras N fallos (10 por defecto) la cuenta se bloquea 10 minutos (no afecta a root) | `/etc/security/faillock.conf` y perfiles PAM `rros-faillock*` |
| sudo | `security.sudo_timeout_minutes` | Minutos que `sudo` recuerda tu contraseña (5; 0 = la pide siempre) | `/etc/sudoers.d/rros` (comprobado con `visudo` antes de instalarlo) |
| Volcados de memoria | `security.core_dumps` | Sin volcados (pueden contener contraseñas) | `/etc/security/limits.d/90-rros-core.conf` |
| Memorias USB | `security.usb_storage` | Permitidas | `/etc/modprobe.d/rros-security.conf` |
| Cámara | `security.camera` | Permitida | `/etc/modprobe.d/rros-security.conf` |
| Actualizaciones | `system.auto_updates` | Las de seguridad se instalan solas cada día | `/etc/apt/apt.conf.d/20auto-upgrades` |
| AppArmor | — | Confinamiento de programas (perfiles de Ubuntu) | — |

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
- El instalador aún no cifra el disco (LUKS): la auditoría lo avisa.
- Falta comprobarlo en hardware real.
