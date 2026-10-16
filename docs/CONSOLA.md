# La consola de RROS paso a paso

Esta guía explica **cómo se escribe cada comando**, qué significa cada parte y qué hacer cuando algo falla. Todos los ejemplos son reales y funcionan tal cual.

Abre una terminal con `Mod+Enter` (en RROS, `Mod` es la tecla Windows/Súper).

---

## 1. Cómo se lee un comando

```
rros config set ui.accent '#22d3ee'
│    │      │   │         └─ valor     lo que quieres poner
│    │      │   └─────────── clave     qué ajuste cambias
│    │      └─────────────── subcomando qué quieres hacer
│    └────────────────────── herramienta
└─────────────────────────── comando principal
```

- **`rros`** es el comando principal. `rros config ...` es lo mismo que `rros-config ...`; `rros theme` es `rros-theme`, y así con las demás.
- **Subcomando**: la acción (`get`, `set`, `list`, `use`...).
- **Argumentos**: lo que necesita la acción (la clave, el valor, un nombre).
- **Opciones**: empiezan por `-` o `--` y modifican el comportamiento (`-p lite`, `--system`, `--yes`). Se escriben **antes del subcomando** cuando son generales: `rros-config -p pro get ui.gap`.
- Los espacios separan las partes. Si un valor lleva espacios, va entre comillas (siguiente sección).

### Los comandos de RROS

| Escribes | Para qué sirve |
|---|---|
| `rros info` | Ficha del sistema: perfil, tema, memoria y disco frente a su presupuesto |
| `rros doctor` | Diagnóstico: configuración válida, memoria, snap, telemetría |
| `rros config` | Leer y cambiar ajustes (`get`, `set`, `list`, `export`...) |
| `rros theme` | Temas: `list`, `use`, `cycle`, `toggle`, `new` |
| `rros settings` | Interfaz gráfica o de terminal de ajustes |
| `rros apply` | Aplica el perfil al sistema (zram, swap, red, servicios) |
| `rros budget` | Comprueba el presupuesto de memoria y disco |
| `rros hw` | Hardware detectado y perfil recomendado |
| `rros install plan` | Calcula las particiones (no toca ningún disco) |
| `rros install` | Instala RROS en un disco desde el sistema live (borra el disco elegido; `--dry-run` solo simula) |
| `rros seguridad` | Auditoría de seguridad con puntuación; niveles y cortafuegos (`rros-security`) |
| `rros dispositivos` | Discos, USB, red, radios, sonido, pantallas, batería; montar y expulsar (`rros-devices`) |
| `rros nuevo` | Crear archivos y proyectos desde plantillas (`rros-new`) |

---

## 2. Las comillas (el error más común)

La terminal corta el texto por los espacios y da significado a algunos símbolos (`#`, `$`, `;`, `&`, `*`). Para pasar un valor tal cual, ponlo entre **comillas simples**:

```sh
rros config set ui.accent '#22d3ee'               # correcto
rros config set ui.accent #22d3ee                 # MAL: la terminal toma # como comentario
rros config set keybind.nav 'Mod+Shift+b exec firefox'   # correcto
rros config set keybind.nav Mod+Shift+b exec firefox     # MAL: son cuatro argumentos
```

Comillas dobles (`"..."`) también valen, pero dejan que la terminal sustituya `$VARIABLES`. Usa las simples salvo que quieras eso.

---

## 3. Pedir ayuda

```sh
rros                       # lista de comandos
rros config help           # ayuda completa de config, con ejemplos
rros theme                 # ayuda de theme (sin subcomando)
rros config schema         # TODOS los ajustes, con tipo, rango y descripción
rros config schema | grep gap          # buscar un ajuste concreto
```

`Tab` completa nombres de archivos; la flecha arriba (`↑`) recupera el comando anterior, y `Ctrl+R` busca en el historial.

---

## 4. Leer ajustes

```sh
rros config get ui.gap          # el valor que está en vigor ahora
rros config list                # todos, indicando de dónde viene cada uno
rros config list | grep ui.     # solo los de interfaz
```

`list` muestra la **capa** de cada valor:

| Capa | Dónde vive | Quién manda |
|---|---|---|
| `perfil` | `/usr/share/rros/profiles/<perfil>.toml` | Es el valor por defecto de Lite, Standard o Pro |
| `sistema` | `/etc/rros/rros.toml` | Cambia a todos los usuarios (`--system`) |
| `usuario` | `~/.config/rros/rros.toml` | **Es el tuyo y gana siempre** |

---

## 5. Cambiar ajustes

```sh
rros config set CLAVE VALOR          # lo cambia (en tu capa de usuario)
rros config unset CLAVE              # lo quita: vuelve al valor del perfil
rros config reset --yes              # quita TODOS tus cambios (guarda copia .bak)
rros config set --system CLAVE VALOR # para todo el sistema (necesita permisos)
```

Cada ajuste tiene un **tipo**, y `set` lo comprueba antes de guardar:

| Tipo | Ejemplo | Qué se acepta |
|---|---|---|
| entero | `rros config set ui.gap 12` | Un número dentro del rango (`rros config schema` lo muestra) |
| booleano | `rros config set ui.blur true` | Solo `true` o `false` |
| opción | `rros config set ui.layout tiling` | Una de las opciones listadas |
| texto | `rros config set ui.font 'Inter'` | Texto libre (hasta 256 caracteres) |

### Los errores y qué significan

```text
rros-config: ui.gap: 9999 fuera de rango [0, 64]
   → el número se sale del rango; usa uno entre 0 y 64.

rros-config: ui.effects: 'mucho' no válido (opciones: off|basic|advanced)
   → escribe una de las opciones que te da entre paréntesis.

rros-config: ui.animations: se esperaba true o false
   → los interruptores solo admiten true o false (no "si", "on" ni 1).

rros-config: clave 'ui.nope' desconocida (ver `rros-config schema`)
   → la clave no existe; revisa el nombre con `rros config schema | grep ...`.

rros-config: 'profile.name' solo es válida en archivos de perfil
   → esa clave define el perfil y no se cambia desde tus ajustes.
```

Un error **nunca** deja el archivo a medias: si `set` falla, tus ajustes siguen como estaban.

Para ver el resultado en el escritorio: `rros theme apply` (los cambios de la interfaz gráfica se aplican solos).

---

## 6. Temas

```sh
rros theme list              # los temas; * marca el que usas
rros theme use nord          # cambia de tema y lo aplica a todo
rros theme cycle             # pasa al siguiente (atajo: Mod+Shift+T)
rros theme toggle            # alterna oscuro/claro (atajo: Mod+T)
rros theme palette           # los colores resultantes
```

Temas incluidos: `aurora`, `nord`, `dracula`, `gruvbox`, `solarized`, `mono` y `contrast` (alto contraste). Cada uno tiene versión oscura y clara. El tema llega a las ventanas, el panel, la terminal, las notificaciones, el lanzador y las aplicaciones GTK.

**Acento propio** (se aplica sobre el tema):

```sh
rros config set ui.accent '#f472b6'
rros config unset ui.accent        # vuelve al acento del tema
```

**Crear tu tema**:

```sh
rros theme new miotema --base nord     # crea ~/.config/rros/themes/miotema.toml
nano ~/.config/rros/themes/miotema.toml   # cambia los colores (#RRGGBB)
rros theme use miotema
```

Lo que no definas en tu tema se hereda del tema base. También puedes editar los colores con selectores en `rros settings`.

### El panel

```sh
rros config set panel.left   'workspaces,window'
rros config set panel.center 'clock'
rros config set panel.right  'cpu,memory,disk,network,audio,battery,tray'
rros config set panel.style  floating        # flat (por defecto), floating o pill
rros config set ui.panel_position bottom     # top, bottom, left o right
```

Los módulos se escriben separados por comas, sin espacios. Disponibles: `workspaces`, `mode`, `window`, `language`, `clock`, `profile`, `memory`, `cpu`, `network`, `audio`, `battery`, `tray`, `disk`, `temperature`, `backlight`, `idle`. Los que no existan se ignoran.

---

## 7. Atajos, reglas de ventana e inicio automático

Estas tres cosas son **claves dinámicas**: tú eliges el identificador (la parte después del punto) y el valor tiene un formato fijo. RROS lo comprueba al guardarlo, así que un valor mal escrito se rechaza con un mensaje.

### Atajos de teclado: `keybind.<id>`

Formato: `COMBINACIÓN COMANDO`

```sh
rros config set keybind.navegador 'Mod+Shift+b exec firefox'
rros config set keybind.captura   'Mod+Shift+p exec rros-shot region'
rros config set keybind.cinco     'Mod+5 workspace number 5'
rros config set keybind.cerrar    'Mod+Shift+q kill'
```

- La combinación une teclas con `+`. `Mod` es la tecla Windows. Otras: `Shift`, `Ctrl`, `Alt`, letras, números, `Return`, `space`, `Print`, `F1`...
- El comando debe empezar por uno de estos: `exec` (lanza un programa), `workspace`, `move`, `focus`, `kill`, `fullscreen`, `floating`, `layout`, `splith`, `splitv`, `reload`, `resize`, `scratchpad`, `sticky`, `mode`.
- Un atajo tuyo con la misma combinación que uno de RROS lo sustituye.

### Reglas de ventana: `rule.<id>`

Formato: `CRITERIO=VALOR ACCIÓN`

```sh
rros config set rule.ff      'app_id=firefox workspace 2'   # Firefox siempre en el escritorio 2
rros config set rule.calc    'title=Calculadora floating'   # la calculadora flota
rros config set rule.video   'app_id=mpv inhibit_idle'      # sin salvapantallas viendo vídeo
rros config set rule.steam   'class=Steam border pixel 0'   # sin borde
rros config set rule.glass   'app_id=foot opacity 0.9'      # algo transparente
```

- Criterios: `app_id` (programas Wayland), `class` (programas X11), `title`, `window_role`. El valor admite letras, números, `.`, `_` y `-`.
- Acciones: `floating`, `sticky`, `fullscreen`, `inhibit_idle`, `workspace N` (1 a 32), `opacity X` (0 a 1), `border none`, `border pixel N` (0 a 16).
- Para saber el `app_id` de una ventana: `swaymsg -t get_tree | grep app_id`.

### Inicio automático: `autostart.<id>`

```sh
rros config set autostart.sync 'syncthing -no-browser'
```

El valor es el comando tal como lo escribirías en la terminal.

### Quitar uno

```sh
rros config unset keybind.navegador
```

Después de cambiar atajos o reglas: `rros theme apply` (recarga sway).

---

## 8. Copias de seguridad de tus ajustes

```sh
rros config snapshot save antes-de-probar    # guarda el estado actual
rros config snapshot list                    # lista las instantáneas
rros config snapshot restore antes-de-probar # vuelve a ese estado
rros config snapshot delete antes-de-probar

rros config export > mis-ajustes.toml        # tus ajustes a un archivo
rros config import mis-ajustes.toml          # los carga (si son válidos)
```

`import` y `restore` guardan antes una copia `rros.toml.bak`, y rechazan archivos con valores no válidos sin tocar nada.

---

## 9. Diagnóstico

```sh
rros doctor                  # [ok] perfil cargado · configuración válida · RAM · snap · telemetría
rros info                    # ficha con barras de memoria y disco
rros budget ram              # ¿la RAM en reposo cabe en el presupuesto del perfil?
rros budget disk /           # ¿y el disco?
rros hw                      # RAM, CPU, GPU y perfil que recomienda
rros hw --json               # lo mismo, para programas
rros config validate         # revisa tus archivos de ajustes
```

Los comandos **devuelven un código**: `0` si todo va bien y otro número si no. Puedes usarlo:

```sh
rros budget ram && echo "cabe"        # solo se muestra si cabe
rros config validate || echo "revisa tus ajustes"
echo $?                                # el código del último comando
```

### Plan de instalación

```sh
rros install plan --disk-gb 64 --uefi -p standard
rros install plan --disk-gb 64 --uefi -p standard --sfdisk   # el script de particiones
```

Solo **calcula**; no escribe en ningún disco.

---

## 10. Tuberías y redirecciones

| Símbolo | Qué hace | Ejemplo |
|---|---|---|
| `\|` | Pasa la salida de un comando al siguiente | `rros config list \| grep panel` |
| `>` | Guarda la salida en un archivo (lo sobrescribe) | `rros config export > copia.toml` |
| `>>` | Añade al final de un archivo | `rros info --plain >> informe.txt` |
| `&&` | Ejecuta el siguiente solo si el anterior fue bien | `rros theme use nord && rros theme toggle` |
| `\|\|` | Ejecuta el siguiente solo si el anterior falló | `rros doctor \|\| echo "hay problemas"` |
| `;` | Ejecuta uno tras otro, pase lo que pase | `rros theme cycle; rros info` |

---

## 11. Crear tus propios comandos

### Alias (atajos de escritura)
Añade a `~/.bashrc` (ya incluye `r`, `ri`, `rc` y `rt`):

```sh
alias noche='rros theme use gruvbox'
alias dia='rros theme use solarized && rros config set ui.color_scheme light'
```
Abre una terminal nueva (o `source ~/.bashrc`) y escribe `noche`.

### Scripts
Un comando es solo un archivo ejecutable. Ejemplo, `~/.local/bin/modo-presentacion`:

```sh
#!/bin/sh
# Oculta distracciones y sube el contraste
rros config set ui.animations false
rros config set ui.color_scheme light
rros theme use contrast
```
```sh
chmod +x ~/.local/bin/modo-presentacion
modo-presentacion
```
(`~/.local/bin` ya está en el PATH en RROS.)

### Extensiones de `rros`
Cualquier programa llamado `rros-NOMBRE` que esté en el PATH funciona como `rros NOMBRE`:

```sh
mv ~/.local/bin/modo-presentacion ~/.local/bin/rros-presentacion
rros presentacion
```
Así tus comandos conviven con los de RROS y aparecen con el mismo estilo.

---

## 12. Seguridad, dispositivos y archivos nuevos

### Seguridad
```sh
rros seguridad                       # puntuación y qué arreglar (cada punto dice la orden exacta)
rros seguridad niveles               # basico, estandar (por defecto) y estricto
sudo rros seguridad nivel estricto   # sin memorias USB ni cámara, sin ping ni SSH...
sudo rros seguridad abrir 8080/tcp   # deja pasar conexiones a ese puerto
sudo rros seguridad cerrar 8080/tcp
```
Las directivas son claves `security.*` del **sistema**: se guardan en `/etc/rros/rros.toml` con `--system` y las aplica `rros-apply` como root. Más en [SEGURIDAD.md](SEGURIDAD.md).
```sh
sudo rros config set --system security.login_attempts 5 && sudo rros apply --services
```

### Dispositivos
```sh
rros dispositivos                    # todo, agrupado
rros dispositivos lista disco        # solo discos y memorias (también: usb red radio sonido pantalla energia entrada camara tarjeta)
rros dispositivos montar sdb1        # monta una memoria USB (sin sudo: lo autoriza udisks)
rros dispositivos expulsar sdb       # desmonta todo y apaga la memoria: ya se puede quitar
rros dispositivos radio wifi off     # apaga el Wi-Fi (bluetooth, todo; on para encender)
```
Los discos del sistema (`/`, `/boot`...) nunca se desmontan desde aquí.

### Crear archivos
```sh
rros nuevo                           # asistente: elige plantilla, nombre y carpeta
rros nuevo lista                     # plantillas de RROS y las tuyas
rros nuevo bash copia-seguridad      # copia-seguridad.sh, ya ejecutable
rros nuevo web index --dir ~/web     # ~/web/index.html
rros nuevo proyecto-c hola           # carpeta hola/ con main.c y Makefile: cd hola && make && ./hola
rros nuevo lanzador "Mi radio"       # acceso directo en el menú de aplicaciones
```
Cualquier archivo que guardes en `~/Plantillas` se convierte en plantilla: aparece en `rros nuevo lista` y en el menú «Crear documento» del gestor de archivos (botón derecho). Nunca se pisa un archivo existente salvo con `--force`.

---

## 13. Perfiles y variables de entorno

Casi todos los comandos aceptan `-p PERFIL` para consultar otro perfil sin cambiar el activo:

```sh
rros config -p lite get ui.effects      # off
rros config -p pro  get ui.effects      # advanced
```

Para pruebas y scripts puedes redirigir dónde lee y escribe RROS:

| Variable | Qué cambia |
|---|---|
| `RROS_USER_CONF` | Tu archivo de ajustes (por defecto `~/.config/rros/rros.toml`) |
| `RROS_SYSTEM_CONF` | El archivo del sistema (`/etc/rros/rros.toml`) |
| `RROS_PROFILE_DIR` | Dónde están los perfiles |
| `RROS_PROFILE_FILE` | Qué archivo dice cuál es el perfil activo (`/etc/rros/profile`) |
| `RROS_THEMES_DIR` | Tu carpeta de temas |
| `RROS_TEMPLATES_DIR` | Plantillas de `rros nuevo` (por defecto `/usr/share/rros/templates`) |
| `RROS_ROOT` | `rros seguridad` y `rros dispositivos` leen `/sys`, `/proc` y `/etc` bajo esta carpeta (pruebas) |
| `XDG_CONFIG_HOME` | Dónde se escriben `sway`, `waybar`, `foot`... |
| `NO_COLOR` | Si existe, `rros info` y `rros doctor` no usan colores |

Ejemplo: probar un cambio sin tocar tus ajustes reales:

```sh
RROS_USER_CONF=/tmp/prueba.toml rros config set ui.gap 30
RROS_USER_CONF=/tmp/prueba.toml rros config get ui.gap     # 30
rros config get ui.gap                                      # tu valor real, intacto
```

---

## 14. Chuleta

| Quiero... | Escribo |
|---|---|
| Ver cómo está mi sistema | `rros info` · `rros doctor` |
| Cambiar de tema | `rros theme use nord` |
| Oscuro / claro | `rros theme toggle` |
| Cambiar el acento | `rros config set ui.accent '#22d3ee'` |
| Más separación entre ventanas | `rros config set ui.gap 16` |
| Panel abajo | `rros config set ui.panel_position bottom` |
| Panel con módulos a mi gusto | `rros config set panel.right 'cpu,memory,battery'` |
| Panel flotante o en píldoras | `rros config set panel.style floating` (o `pill`) |
| Fondo de pantalla | `rros config set ui.wallpaper /ruta/foto.jpg` |
| Un atajo | `rros config set keybind.x 'Mod+b exec firefox'` |
| Una regla de ventana | `rros config set rule.x 'app_id=firefox workspace 2'` |
| Iniciar un programa al entrar | `rros config set autostart.x 'comando'` |
| Deshacer un cambio | `rros config unset CLAVE` |
| Guardar/volver a un estado | `rros config snapshot save/restore NOMBRE` |
| Ajustes con ventana | `rros settings` (o `Mod+,`) |
| Ajustes en la terminal | `rros settings tui` |
| Buscar un ajuste | `rros config schema \| grep palabra` |
| ¿Estoy protegido? | `rros seguridad` |
| Máxima seguridad | `sudo rros seguridad nivel estricto` |
| Abrir un puerto | `sudo rros seguridad abrir 8080/tcp` |
| Ver mis dispositivos | `rros dispositivos` |
| Quitar una memoria USB | `rros dispositivos expulsar sdb` |
| Apagar el Wi-Fi | `rros dispositivos radio wifi off` |
| Crear un script | `rros nuevo bash nombre` |
| Empezar un proyecto | `rros nuevo proyecto-python nombre` |
