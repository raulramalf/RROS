# La consola de Lily paso a paso

Esta guía explica **cómo se escribe cada comando**, qué significa cada parte y qué hacer cuando algo falla. Todos los ejemplos son reales y funcionan tal cual.

Abre una terminal desde el dock (el icono de Konsole) o con Ctrl+Alt+T.

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

### Los comandos de Lily

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
| `rros install` | Instala Lily desde el sistema live: borrando el disco o junto a Windows, con cifrado opcional (`--dry-run` solo simula). También en Ajustes de Lily → *Instalar Lily* |
| `rros informe` | Informe de hardware para contar cómo funciona Lily en tu PC (sin números de serie ni MAC) |
| `rros seguridad` | Auditoría de seguridad con puntuación; niveles y cortafuegos (`rros-security`) |
| `rros dispositivos` | Discos, USB, red, radios, sonido, pantallas, batería; montar y expulsar (`rros-devices`) |
| `rros nuevo` | Crear archivos y proyectos desde plantillas (`rros-new`) |

---

## 2. Las comillas (el error más común)

La terminal corta el texto por los espacios y da significado a algunos símbolos (`#`, `$`, `;`, `&`, `*`). Para pasar un valor tal cual, ponlo entre **comillas simples**:

```sh
rros config set ui.accent '#22d3ee'               # correcto
rros config set ui.accent #22d3ee                 # MAL: la terminal toma # como comentario
rros config set keybind.nav 'Mod+Shift+b firefox'   # correcto
rros config set keybind.nav Mod+Shift+b firefox     # MAL: son tres argumentos
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
rros theme cycle             # pasa al siguiente
rros theme toggle            # alterna oscuro/claro
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

### Escritorio, dock y fondo

El escritorio es KDE Plasma con el aspecto de Lily. Tres estilos preparados cambian de golpe la barra y el dock:

```sh
rros theme escritorio              # los estilos; * marca el que usas
rros theme escritorio moderno      # el de Lily: barra fina arriba con el menú Lily y dock abajo
rros theme escritorio clasico      # barra de tareas abajo con el botón de aplicaciones, sin dock
rros theme escritorio enfoque      # barra en píldora con el reloj en el centro y el dock escondido
```

El dock, uno a uno:

```sh
rros config set ui.dock_style 3d           # 2d (plano) o 3d (brillo, bisel y canto)
rros config set ui.dock_icon_size 56       # de 32 a 96 px
rros config set ui.dock_position left      # bottom, left o right
rros config set ui.dock_hiding auto        # never, dodge (se aparta de las ventanas) o auto
rros config set ui.dock_launchpad false    # sin el botón del Launchpad
rros config set ui.window_buttons right    # botones de las ventanas a la derecha
rros config set ui.global_menu false       # el menú de cada aplicación en su ventana
```

El fondo se genera con los colores de tu tema, así que cambia solo al cambiar de tema:

```sh
rros theme wallpaper --next                     # siguiente estilo
rros config set ui.wallpaper_style ondas        # aurora, ondas, degradado, circulos o liso
rros config set ui.wallpaper ~/Imágenes/mar.jpg # tu propia imagen
rros theme wallpaper --style circulos --size 1920x1080 --out ~/fondo.png   # guardarlo como imagen
```

Efectos (también en Ajustes → *Escritorio y Dock*):

```sh
rros config set ui.effects advanced        # off, basic o advanced
rros config set ui.animation_style rebote  # suave, rapida, rebote (lámpara mágica) o deslizar
rros config set ui.blur true               # desenfoque detrás de menús y paneles (con efectos «advanced»)
rros config set ui.dim_inactive true       # oscurece las ventanas sin foco
rros config set ui.hot_corners false       # sin esquina activa
```

Sin aceleración gráfica (una máquina virtual sin 3D) los efectos se apagan solos.

### La barra superior

```sh
rros config set panel.style pill           # flat (de borde a borde), floating o pill
rros config set ui.panel_position bottom   # top, bottom, left o right
rros config set ui.panel_size 32
rros config set ui.clock_24h false
```

Cambiar la barra o el dock los rehace al momento si estás en Plasma.

---

## 7. Atajos, reglas de ventana e inicio automático

Estas tres cosas son **claves dinámicas**: tú eliges el identificador (la parte después del punto) y el valor tiene un formato fijo. Lily lo comprueba al guardarlo, así que un valor mal escrito se rechaza con un mensaje.

### Atajos de teclado: `keybind.<id>`

Formato: `COMBINACIÓN PROGRAMA`

```sh
rros config set keybind.navegador 'Mod+Shift+b firefox'
rros config set keybind.notas     'Ctrl+Alt+n kate ~/Documentos/notas.txt'
```

- La combinación une teclas con `+`. `Mod` es la tecla Super (la del logotipo). Otras: `Shift`, `Ctrl`, `Alt`, letras, números, `Return`, `space`, `Print`, `F1`...
- El atajo abre el programa. Para los atajos del propio escritorio (cerrar ventanas, escritorios...), usa Ajustes → *Teclado y atajos* → *Todos los atajos de KDE*.
- Funcionan desde el siguiente inicio de sesión.

### Reglas de ventana: `rule.<id>`

Formato: `app=NOMBRE ACCIÓN` (o `title=TÍTULO ACCIÓN`)

```sh
rros config set rule.video 'app=mpv above'           # siempre encima
rros config set rule.ff    'app=firefox maximized'   # Firefox se abre maximizado
rros config set rule.juego 'app=steam fullscreen'
rros config set rule.glass 'app=konsole opacity 90'  # algo transparente
```

- Acciones: `above`, `maximized`, `fullscreen`, `noborder`, `skiptaskbar`, `minimized` y `opacity N` (10 a 100).
- El nombre es la clase de la ventana (normalmente el nombre del programa); se busca como parte del nombre.

### Inicio automático: `autostart.<id>`

```sh
rros config set autostart.sync 'syncthing -no-browser'
```

El valor es el comando tal como lo escribirías en la terminal.

### Quitar uno

```sh
rros config unset keybind.navegador
```

Después de cambiar atajos o reglas: `rros theme apply`.

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
rros-installer disks                                  # discos: tamaño, tabla, espacio libre, Windows/Linux
rros install plan --disk-gb 64 --uefi -p standard
rros install plan --disk-gb 64 --uefi -p standard --sfdisk   # el script de particiones
rros install plan --disk /dev/nvme0n1 --alongside --encrypt   # junto a Windows y cifrado
```

Solo **calcula**; no escribe en ningún disco.

### Instalar

```sh
rros install                                     # asistente de texto: disco, modo, cifrado, usuario
rros install --disk /dev/nvme0n1 --alongside --encrypt \
     --user ana --fullname "Ana Pérez" --hostname ana-pc \
     --keymap es --locale es_ES.UTF-8 --timezone Europe/Madrid \
     --password-file /tmp/pw --passphrase-file /tmp/disco --yes
```

| Opción | Qué hace |
|---|---|
| `--alongside` | Usa el mayor espacio libre del disco (GPT) y deja lo demás como está; reutiliza la partición EFI de Windows y añade Windows al menú de arranque (os-prober) |
| `--encrypt` | Cifra el sistema con LUKS2; `/boot` va aparte sin cifrar (GRUB no abre LUKS2). Al encender se pide la contraseña con tu distribución de teclado |
| `--progress-file F` | Escribe el paso actual en F (lo usa el asistente gráfico) |

Si la instalación falla junto a otro sistema, quita las particiones que había creado.

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
(`~/.local/bin` ya está en el PATH en Lily.)

### Extensiones de `rros`
Cualquier programa llamado `rros-NOMBRE` que esté en el PATH funciona como `rros NOMBRE`:

```sh
mv ~/.local/bin/modo-presentacion ~/.local/bin/rros-presentacion
rros presentacion
```
Así tus comandos conviven con los de Lily y aparecen con el mismo estilo.

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
rros nuevo lista                     # plantillas de Lily y las tuyas
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

Para pruebas y scripts puedes redirigir dónde lee y escribe Lily:

| Variable | Qué cambia |
|---|---|
| `RROS_USER_CONF` | Tu archivo de ajustes (por defecto `~/.config/rros/rros.toml`) |
| `RROS_SYSTEM_CONF` | El archivo del sistema (`/etc/rros/rros.toml`) |
| `RROS_PROFILE_DIR` | Dónde están los perfiles |
| `RROS_PROFILE_FILE` | Qué archivo dice cuál es el perfil activo (`/etc/rros/profile`) |
| `RROS_THEMES_DIR` | Tu carpeta de temas |
| `RROS_TEMPLATES_DIR` | Plantillas de `rros nuevo` (por defecto `/usr/share/rros/templates`) |
| `RROS_ROOT` | `rros seguridad` y `rros dispositivos` leen `/sys`, `/proc` y `/etc` bajo esta carpeta (pruebas) |
| `XDG_CONFIG_HOME` | Dónde se escriben `kwinrc`, `kdeglobals`, `kcminputrc`... |
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
| Un atajo | `rros config set keybind.x 'Mod+b firefox'` |
| Una regla de ventana | `rros config set rule.x 'app=mpv above'` |
| Iniciar un programa al entrar | `rros config set autostart.x 'comando'` |
| Deshacer un cambio | `rros config unset CLAVE` |
| Guardar/volver a un estado | `rros config snapshot save/restore NOMBRE` |
| Ajustes con ventana | `rros settings` (o el menú Lily → Ajustes) |
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
