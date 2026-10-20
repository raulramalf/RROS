# Personalización

Generado con `rros-config schema --markdown`. Capas: perfil < sistema < usuario.

| Clave | Tipo | Valores | Descripción |
|---|---|---|---|
| `ui.theme` | texto |  | Tema visual (nombre de tema instalado) |
| `ui.icon_theme` | texto |  | Tema de iconos |
| `ui.cursor_theme` | texto |  | Tema de cursor |
| `ui.cursor_size` | entero | 12 – 128 | Tamaño del cursor (px) |
| `ui.accent` | texto |  | Color de acento, formato #RRGGBB |
| `ui.color_scheme` | opción | light|dark|auto | Esquema de color |
| `ui.scale` | entero | 50 – 400 | Escala de interfaz (%) |
| `ui.fractional_scale_step` | entero | 5 – 50 | Paso del escalado fraccional (%) |
| `ui.font` | texto |  | Fuente de interfaz |
| `ui.font_mono` | texto |  | Fuente monoespaciada |
| `ui.font_size` | entero | 6 – 32 | Tamaño de fuente (pt) |
| `ui.font_hinting` | opción | none|slight|medium|full | Hinting de fuentes |
| `ui.font_antialias` | opción | none|grayscale|rgba | Suavizado de fuentes |
| `ui.wallpaper` | texto |  | Ruta del fondo de pantalla |
| `ui.wallpaper_mode` | opción | fill|fit|stretch|center|tile | Ajuste del fondo |
| `ui.wallpaper_style` | opción | aurora|ondas|degradado|circulos|petalos|dunas|malla|cristal|topografico|seda|liso | Fondo generado con los colores del tema (si no eliges una imagen) |
| `ui.effects` | opción | off|basic|advanced | Nivel de efectos visuales |
| `ui.animations` | booleano | true / false | Animaciones de ventanas y menús |
| `ui.animation_speed` | entero | 25 – 400 | Velocidad de animación (%) |
| `ui.animation_style` | opción | suave|rapida|rebote|deslizar | Estilo de las animaciones de ventanas y escritorios |
| `ui.transparency` | entero | 0 – 100 | Transparencia de paneles y ventanas (%) |
| `ui.blur` | booleano | true / false | Desenfoque de fondo |
| `ui.blur_strength` | entero | 1 – 20 | Intensidad del desenfoque |
| `ui.shadows` | booleano | true / false | Sombras de ventanas |
| `ui.dim_inactive` | booleano | true / false | Oscurecer un poco las ventanas sin foco |
| `ui.corner_radius` | entero | 0 – 32 | Radio de esquinas (px) |
| `ui.taskbar_style` | opción | rros|clasica|isla|dock|doble|lateral | Barra de tareas: Fusión (la de RROS), Clásica, Isla flotante, Dock con barra superior, Doble o Lateral |
| `ui.taskbar_align` | opción | auto|centro|izquierda | Aplicaciones de la barra de tareas: en el centro o a la izquierda |
| `ui.taskbar_size` | entero | 36 – 72 | Alto de la barra de tareas (px) |
| `ui.taskbar_labels` | booleano | true / false | Nombre de cada ventana junto a su icono en la barra de tareas |
| `ui.taskbar_search` | booleano | true / false | Buscador en la barra de tareas |
| `ui.dock` | booleano | true / false | Dock de aplicaciones (estilo Dock) |
| `ui.dock_style` | opción | 2d|3d | Aspecto del dock: plano (2d) o con relieve, brillo y reflejo (3d) |
| `ui.dock_icon_size` | entero | 32 – 96 | Tamaño de los iconos del dock (px) |
| `ui.dock_position` | opción | bottom|left|right | Posición del dock |
| `ui.dock_hiding` | opción | never|dodge|auto | Dock: siempre visible, se aparta de las ventanas o se oculta solo |
| `ui.dock_apps` | texto |  | Dock: aplicaciones fijadas, en orden (ids de .desktop separados por comas) |
| `ui.dock_launchpad` | booleano | true / false | Botón Launchpad (todas las aplicaciones en pantalla completa) en el dock |
| `ui.window_buttons` | opción | left|right | Botones de las ventanas (cerrar, minimizar, maximizar) a la izquierda o a la derecha |
| `ui.global_menu` | booleano | true / false | Menú de la aplicación en la barra superior, como en macOS |
| `ui.panel_position` | opción | top|bottom|left|right | Posición del panel |
| `ui.panel_size` | entero | 16 – 128 | Tamaño del panel (px) |
| `ui.panel_autohide` | booleano | true / false | Ocultar panel automáticamente |
| `ui.clock_24h` | booleano | true / false | Reloj de 24 horas |
| `ui.show_battery_percent` | booleano | true / false | Mostrar porcentaje de batería |
| `ui.workspaces` | entero | 1 – 32 | Número de escritorios virtuales |
| `ui.hot_corners` | booleano | true / false | Esquinas activas |
| `ui.touch_mode` | booleano | true / false | Interfaz optimizada para pantalla táctil |
| `ui.high_contrast` | booleano | true / false | Alto contraste |
| `ui.reduce_motion` | booleano | true / false | Reducir movimiento (accesibilidad) |
| `ui.night_light` | opción | off|auto|on | Filtro de luz nocturna |
| `ui.night_light_temp` | entero | 1000 – 6500 | Temperatura del filtro nocturno (K) |
| `apps.terminal` | texto |  | Terminal (comando) |
| `apps.browser` | texto |  | Navegador (comando) |
| `apps.files` | texto |  | Gestor de archivos (comando) |
| `apps.editor` | texto |  | Editor de texto (comando) |
| `panel.style` | opción | flat|floating|pill | Estilo de la barra: de borde a borde, flotante o en píldora |
| `panel.taskbar` | booleano | true / false | Mostrar las ventanas abiertas en el panel (clic: ir a ella; clic central: cerrarla) |
| `term.scrollback` | entero | 0 – 100000 | Líneas de historial de la terminal |
| `term.cursor` | opción | block|beam|underline | Forma del cursor de la terminal |
| `term.padding` | entero | 0 – 40 | Margen interior de la terminal (px) |
| `display.vrr` | opción | off|auto|on | Frecuencia variable (VRR) |
| `display.refresh_hz` | entero | 24 – 540 | Frecuencia de refresco preferida (Hz) |
| `display.rotation` | opción | 0|90|180|270 | Rotación de pantalla |
| `display.brightness` | entero | 1 – 100 | Brillo inicial (%) |
| `input.keymap` | texto |  | Distribución de teclado (p. ej. es, us, latam) |
| `input.repeat_delay_ms` | entero | 100 – 2000 | Retardo de repetición de tecla (ms) |
| `input.repeat_rate` | entero | 1 – 100 | Repeticiones por segundo |
| `input.natural_scroll` | booleano | true / false | Desplazamiento natural |
| `input.tap_to_click` | booleano | true / false | Pulsar para hacer clic |
| `input.disable_while_typing` | booleano | true / false | Desactivar touchpad al escribir |
| `input.pointer_speed` | entero | -100 – 100 | Velocidad del puntero |
| `input.numlock_on_boot` | booleano | true / false | Bloq Num al iniciar |
| `input.gestures` | booleano | true / false | Gestos del touchpad: deslizar con tres dedos cambia de escritorio |
| `power.mode` | opción | saver|balanced|performance | Modo de energía |
| `power.screen_off_minutes` | entero | 0 – 1440 | Apagar pantalla tras N minutos (0 = nunca) |
| `power.suspend_minutes` | entero | 0 – 1440 | Suspender tras N minutos (0 = nunca) |
| `power.lid_action` | opción | ignore|suspend|hibernate|poweroff | Acción al cerrar la tapa |
| `power.cpu_boost` | booleano | true / false | Permitir turbo de CPU |
| `power.battery_low_percent` | entero | 3 – 50 | Aviso de batería baja (%) |
| `system.swap` | opción | none|zram|file | Estrategia de swap |
| `system.zram_percent` | entero | 10 – 100 | Tamaño de zram (% de la RAM) |
| `system.zram_algo` | opción | lzo|lz4|zstd | Algoritmo de compresión de zram |
| `system.swappiness` | entero | 0 – 200 | vm.swappiness |
| `system.io_scheduler` | opción | auto|none|mq-deadline|bfq|kyber | Planificador de E/S |
| `system.auto_updates` | booleano | true / false | Actualizaciones automáticas |
| `system.update_channel` | opción | stable|testing|rolling | Canal de actualizaciones |
| `system.trim` | booleano | true / false | TRIM periódico en SSD |
| `services.bluetooth` | booleano | true / false | Bluetooth |
| `services.printing` | booleano | true / false | Impresión (CUPS) |
| `services.indexer` | booleano | true / false | Indexador de archivos |
| `services.ssh` | booleano | true / false | Servidor SSH |
| `services.sound_server` | booleano | true / false | Servidor de sonido |
| `services.location` | booleano | true / false | Servicios de ubicación |
| `services.flatpak` | booleano | true / false | Soporte Flatpak |
| `net.hostname` | texto |  | Nombre del equipo |
| `net.dns` | texto |  | Servidores DNS separados por comas (vacío = automático) |
| `net.wifi_powersave` | booleano | true / false | Ahorro de energía Wi-Fi |
| `net.mac_randomize` | booleano | true / false | Aleatorizar dirección MAC |
| `locale.language` | texto |  | Idioma (p. ej. es_ES.UTF-8) |
| `locale.timezone` | texto |  | Zona horaria (p. ej. Europe/Madrid) |
| `locale.units` | opción | metric|imperial | Sistema de unidades |
| `audio.volume` | entero | 0 – 150 | Volumen inicial (%) |
| `audio.startup_sound` | booleano | true / false | Sonido de inicio |
| `audio.sound_theme` | opción | ocean|freedesktop | Tema de sonidos del sistema |
| `audio.max_volume` | entero | 100 – 150 | Volumen máximo (%): por encima de 100 amplifica |
| `audio.volume_step` | entero | 1 – 20 | Paso del volumen con las teclas (%) |
| `audio.volume_feedback` | booleano | true / false | Sonido de aviso al cambiar el volumen |
| `audio.event_sounds` | booleano | true / false | Sonidos de eventos |
| `privacy.telemetry` | booleano | true / false | Telemetría (desactivada por defecto) |
| `privacy.crash_reports` | booleano | true / false | Informes de fallos |
| `privacy.lock_minutes` | entero | 0 – 1440 | Bloquear pantalla tras N minutos (0 = nunca) |
| `privacy.disk_encryption` | booleano | true / false | Cifrado de disco (LUKS) en la instalación |
| `access.screen_reader` | booleano | true / false | Lector de pantalla (Orca) al iniciar sesión; Super+Alt+S lo activa o desactiva |
| `access.onscreen_keyboard` | booleano | true / false | Teclado en pantalla al iniciar sesión; Super+Alt+K lo muestra u oculta |
| `security.firewall` | booleano | true / false | Cortafuegos: bloquea las conexiones entrantes no pedidas |
| `security.firewall_ping` | booleano | true / false | Responder a ping con el cortafuegos activo |
| `security.firewall_allow` | texto |  | Puertos abiertos, p. ej. 8080/tcp,5000-5010/udp (vacío = ninguno) |
| `security.kernel_hardening` | booleano | true / false | Endurecer el kernel (punteros, dmesg, BPF, ptrace, redirecciones) |
| `security.usb_storage` | booleano | true / false | Permitir memorias y discos USB |
| `security.camera` | booleano | true / false | Permitir cámaras web |
| `security.login_attempts` | entero | 0 – 20 | Intentos de contraseña antes de bloquear 10 minutos (0 = sin límite) |
| `security.sudo_timeout_minutes` | entero | 0 – 60 | Minutos que sudo recuerda tu contraseña (0 = la pide siempre) |
| `security.core_dumps` | booleano | true / false | Guardar volcados de memoria de programas que fallan |
| `boot.timeout_seconds` | entero | 0 – 60 | Tiempo del menú de arranque |
| `boot.splash` | booleano | true / false | Pantalla de arranque gráfica |
| `boot.quiet` | booleano | true / false | Arranque silencioso |
| `keybind.<id>` | texto | libre, validado | Atajo: "COMBINACION COMANDO", p. ej. "Mod+Shift+b firefox" |
| `rule.<id>` | texto | libre, validado | Regla de ventana: "app=NOMBRE ACCION" (above, fullscreen, maximized, noborder, skiptaskbar, minimized u opacity N), p. ej. "app=mpv above" |
| `autostart.<id>` | texto | libre, validado | Programa que se inicia con el escritorio |
