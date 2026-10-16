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
| `ui.compositor` | opción | minimal|full | Compositor Wayland |
| `ui.effects` | opción | off|basic|advanced | Nivel de efectos visuales |
| `ui.animations` | booleano | true / false | Animaciones de ventanas y menús |
| `ui.animation_speed` | entero | 25 – 400 | Velocidad de animación (%) |
| `ui.transparency` | entero | 0 – 100 | Transparencia de paneles y ventanas (%) |
| `ui.blur` | booleano | true / false | Desenfoque de fondo |
| `ui.shadows` | booleano | true / false | Sombras de ventanas |
| `ui.corner_radius` | entero | 0 – 32 | Radio de esquinas (px) |
| `ui.gap` | entero | 0 – 64 | Separación entre ventanas (px) |
| `ui.border_width` | entero | 0 – 16 | Grosor de borde de ventana (px) |
| `ui.layout` | opción | floating|tiling|hybrid | Modo de gestión de ventanas |
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
| `services.firewall` | booleano | true / false | Cortafuegos |
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
| `audio.event_sounds` | booleano | true / false | Sonidos de eventos |
| `privacy.telemetry` | booleano | true / false | Telemetría (desactivada por defecto) |
| `privacy.crash_reports` | booleano | true / false | Informes de fallos |
| `privacy.lock_minutes` | entero | 0 – 1440 | Bloquear pantalla tras N minutos (0 = nunca) |
| `privacy.disk_encryption` | booleano | true / false | Cifrado de disco (LUKS) en la instalación |
| `boot.timeout_seconds` | entero | 0 – 60 | Tiempo del menú de arranque |
| `boot.splash` | booleano | true / false | Pantalla de arranque gráfica |
| `boot.quiet` | booleano | true / false | Arranque silencioso |
