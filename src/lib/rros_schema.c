#include "rros_schema.h"
#include "rros_security.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define S(k, d)            { k, RROS_T_STR, 0, 0, NULL, 0, d }
#define I(k, lo, hi, d)    { k, RROS_T_INT, lo, hi, NULL, 0, d }
#define B(k, d)            { k, RROS_T_BOOL, 0, 0, NULL, 0, d }
#define E(k, ch, d)        { k, RROS_T_ENUM, 0, 0, ch, 0, d }
#define PI(k, lo, hi, d)   { k, RROS_T_INT, lo, hi, NULL, 1, d }
#define PS(k, d)           { k, RROS_T_STR, 0, 0, NULL, 1, d }
#define PE(k, ch, d)       { k, RROS_T_ENUM, 0, 0, ch, 1, d }
#define PB(k, d)           { k, RROS_T_BOOL, 0, 0, NULL, 1, d }

const struct rros_key rros_schema[] = {
	/* ---- definición del perfil (solo archivos de perfil) ---- */
	PS("profile.name", "Nombre del perfil"),
	PI("profile.min_ram_mb", 256, 262144, "RAM mínima del equipo (MB)"),
	PI("profile.max_idle_ram_mb", 128, 8192, "Presupuesto de RAM del SO en reposo (MB)"),
	PI("profile.max_disk_gb", 1, 20, "Presupuesto de disco de la instalación (GB)"),
	PE("build.libc", "glibc|musl", "libc del sistema base"),
	PE("build.kernel", "lite|standard|pro", "Fragmento de configuración del kernel"),
	PB("build.multilib", "Incluir librerías 32 bits"),

	/* ---- interfaz ---- */
	S("ui.theme", "Tema visual (nombre de tema instalado)"),
	S("ui.icon_theme", "Tema de iconos"),
	S("ui.cursor_theme", "Tema de cursor"),
	I("ui.cursor_size", 12, 128, "Tamaño del cursor (px)"),
	S("ui.accent", "Color de acento, formato #RRGGBB"),
	E("ui.color_scheme", "light|dark|auto", "Esquema de color"),
	I("ui.scale", 50, 400, "Escala de interfaz (%)"),
	I("ui.fractional_scale_step", 5, 50, "Paso del escalado fraccional (%)"),
	S("ui.font", "Fuente de interfaz"),
	S("ui.font_mono", "Fuente monoespaciada"),
	I("ui.font_size", 6, 32, "Tamaño de fuente (pt)"),
	E("ui.font_hinting", "none|slight|medium|full", "Hinting de fuentes"),
	E("ui.font_antialias", "none|grayscale|rgba", "Suavizado de fuentes"),
	S("ui.wallpaper", "Ruta del fondo de pantalla"),
	E("ui.wallpaper_mode", "fill|fit|stretch|center|tile", "Ajuste del fondo"),
	S("ui.wallpaper_video", "Fondo en vídeo o GIF (ruta a un .mp4, .webm, .mkv o .gif; vacío = fondo fijo)"),
	E("ui.wallpaper_live", "off|petalos|luciernagas|lluvia", "Escena animada sobre el fondo"),
	B("ui.wallpaper_pause_battery", "Parar el fondo animado con la batería"),
	E("ui.wallpaper_style", "aurora|ondas|degradado|circulos|petalos|dunas|malla|cristal|topografico|seda|liso", "Fondo generado con los colores del tema (si no eliges una imagen)"),
	E("ui.effects", "off|basic|advanced", "Nivel de efectos visuales"),
	B("ui.animations", "Animaciones de ventanas y menús"),
	I("ui.animation_speed", 25, 400, "Velocidad de animación (%)"),
	E("ui.animation_style", "suave|rapida|rebote|deslizar", "Estilo de las animaciones de ventanas y escritorios"),
	I("ui.transparency", 0, 100, "Transparencia de paneles y ventanas (%)"),
	B("ui.blur", "Desenfoque de fondo"),
	I("ui.blur_strength", 1, 20, "Intensidad del desenfoque"),
	B("ui.shadows", "Sombras de ventanas"),
	B("ui.dim_inactive", "Oscurecer un poco las ventanas sin foco"),
	I("ui.corner_radius", 0, 32, "Radio de esquinas (px)"),
	E("ui.taskbar_style", "rros|clasica|isla|dock|doble|lateral",
	  "Barra de tareas: Fusión (la de RROS), Clásica, Isla flotante, Dock con barra superior, Doble o Lateral"),
	E("ui.taskbar_align", "auto|centro|izquierda", "Aplicaciones de la barra de tareas: en el centro o a la izquierda"),
	I("ui.taskbar_size", 36, 72, "Alto de la barra de tareas (px)"),
	B("ui.taskbar_labels", "Nombre de cada ventana junto a su icono en la barra de tareas"),
	B("ui.taskbar_search", "Buscador en la barra de tareas"),
	B("ui.dock", "Dock de aplicaciones (estilo Dock)"),
	E("ui.dock_style", "2d|3d", "Aspecto del dock: plano (2d) o con relieve, brillo y reflejo (3d)"),
	I("ui.dock_icon_size", 32, 96, "Tamaño de los iconos del dock (px)"),
	E("ui.dock_position", "bottom|left|right", "Posición del dock"),
	E("ui.dock_hiding", "never|dodge|auto", "Dock: siempre visible, se aparta de las ventanas o se oculta solo"),
	S("ui.dock_apps", "Dock: aplicaciones fijadas, en orden (ids de .desktop separados por comas)"),
	B("ui.dock_launchpad", "Botón Launchpad (todas las aplicaciones en pantalla completa) en el dock"),
	E("ui.window_buttons", "left|right", "Botones de las ventanas (cerrar, minimizar, maximizar) a la izquierda o a la derecha"),
	B("ui.global_menu", "Menú de la aplicación en la barra superior, como en macOS"),
	E("ui.panel_position", "top|bottom|left|right", "Posición del panel"),
	I("ui.panel_size", 16, 128, "Tamaño del panel (px)"),
	B("ui.panel_autohide", "Ocultar panel automáticamente"),
	B("ui.clock_24h", "Reloj de 24 horas"),
	B("ui.show_battery_percent", "Mostrar porcentaje de batería"),
	I("ui.workspaces", 1, 32, "Número de escritorios virtuales"),
	B("ui.hot_corners", "Esquinas activas"),
	B("ui.touch_mode", "Interfaz optimizada para pantalla táctil"),
	B("ui.high_contrast", "Alto contraste"),
	B("ui.reduce_motion", "Reducir movimiento (accesibilidad)"),
	E("ui.night_light", "off|auto|on", "Filtro de luz nocturna"),
	I("ui.night_light_temp", 1000, 6500, "Temperatura del filtro nocturno (K)"),

	/* ---- aplicaciones por defecto ---- */
	S("apps.terminal", "Terminal (comando)"),
	S("apps.browser", "Navegador (comando)"),
	S("apps.files", "Gestor de archivos (comando)"),
	S("apps.editor", "Editor de texto (comando)"),

	/* ---- barra superior ---- */
	E("panel.style", "flat|floating|pill", "Estilo de la barra: de borde a borde, flotante o en píldora"),
	B("panel.taskbar", "Mostrar las ventanas abiertas en el panel (clic: ir a ella; clic central: cerrarla)"),

	/* ---- terminal ---- */
	I("term.scrollback", 0, 100000, "Líneas de historial de la terminal"),
	E("term.cursor", "block|beam|underline", "Forma del cursor de la terminal"),
	I("term.padding", 0, 40, "Margen interior de la terminal (px)"),

	/* ---- pantalla ---- */
	E("display.vrr", "off|auto|on", "Frecuencia variable (VRR)"),
	I("display.refresh_hz", 24, 540, "Frecuencia de refresco preferida (Hz)"),
	E("display.rotation", "0|90|180|270", "Rotación de pantalla"),
	I("display.brightness", 1, 100, "Brillo inicial (%)"),

	/* ---- entrada ---- */
	S("input.keymap", "Distribución de teclado (p. ej. es, us, latam)"),
	I("input.repeat_delay_ms", 100, 2000, "Retardo de repetición de tecla (ms)"),
	I("input.repeat_rate", 1, 100, "Repeticiones por segundo"),
	B("input.natural_scroll", "Desplazamiento natural"),
	B("input.tap_to_click", "Pulsar para hacer clic"),
	B("input.disable_while_typing", "Desactivar touchpad al escribir"),
	I("input.pointer_speed", -100, 100, "Velocidad del puntero"),
	B("input.numlock_on_boot", "Bloq Num al iniciar"),
	B("input.gestures", "Gestos del touchpad: deslizar con tres dedos cambia de escritorio"),

	/* ---- energía ---- */
	E("power.mode", "saver|balanced|performance", "Modo de energía"),
	I("power.screen_off_minutes", 0, 1440, "Apagar pantalla tras N minutos (0 = nunca)"),
	I("power.suspend_minutes", 0, 1440, "Suspender tras N minutos (0 = nunca)"),
	E("power.lid_action", "ignore|suspend|hibernate|poweroff", "Acción al cerrar la tapa"),
	B("power.cpu_boost", "Permitir turbo de CPU"),
	I("power.battery_low_percent", 3, 50, "Aviso de batería baja (%)"),

	/* ---- sistema y memoria ---- */
	E("system.swap", "none|zram|file", "Estrategia de swap"),
	I("system.zram_percent", 10, 100, "Tamaño de zram (% de la RAM)"),
	E("system.zram_algo", "lzo|lz4|zstd", "Algoritmo de compresión de zram"),
	I("system.swappiness", 0, 200, "vm.swappiness"),
	E("system.io_scheduler", "auto|none|mq-deadline|bfq|kyber", "Planificador de E/S"),
	B("system.auto_updates", "Actualizaciones automáticas"),
	E("system.update_channel", "stable|testing|rolling", "Canal de actualizaciones"),
	B("system.trim", "TRIM periódico en SSD"),

	/* ---- servicios (cada servicio activo consume RAM) ---- */
	B("services.bluetooth", "Bluetooth"),
	B("services.printing", "Impresión (CUPS)"),
	B("services.indexer", "Indexador de archivos"),
	B("services.ssh", "Servidor SSH"),
	B("services.sound_server", "Servidor de sonido"),
	B("services.location", "Servicios de ubicación"),
	B("services.flatpak", "Soporte Flatpak"),

	/* ---- red ---- */
	S("net.hostname", "Nombre del equipo"),
	S("net.dns", "Servidores DNS separados por comas (vacío = automático)"),
	B("net.wifi_powersave", "Ahorro de energía Wi-Fi"),
	B("net.mac_randomize", "Aleatorizar dirección MAC"),

	/* ---- región ---- */
	S("locale.language", "Idioma (p. ej. es_ES.UTF-8)"),
	S("locale.timezone", "Zona horaria (p. ej. Europe/Madrid)"),
	E("locale.units", "metric|imperial", "Sistema de unidades"),

	/* ---- sonido ---- */
	I("audio.volume", 0, 150, "Volumen inicial (%)"),
	B("audio.startup_sound", "Sonido de inicio"),
	E("audio.sound_theme", "ocean|freedesktop", "Tema de sonidos del sistema"),
	I("audio.max_volume", 100, 150, "Volumen máximo (%): por encima de 100 amplifica"),
	I("audio.volume_step", 1, 20, "Paso del volumen con las teclas (%)"),
	B("audio.volume_feedback", "Sonido de aviso al cambiar el volumen"),
	B("audio.event_sounds", "Sonidos de eventos"),

	/* ---- privacidad ---- */
	B("privacy.telemetry", "Telemetría (desactivada por defecto)"),
	B("privacy.crash_reports", "Informes de fallos"),
	I("privacy.lock_minutes", 0, 1440, "Bloquear pantalla tras N minutos (0 = nunca)"),
	B("privacy.disk_encryption", "Cifrado de disco (LUKS) en la instalación"),

	/* ---- accesibilidad (además de ui.high_contrast, ui.reduce_motion, ui.font_size, ui.scale y ui.cursor_size) ---- */
	B("access.screen_reader", "Lector de pantalla (Orca) al iniciar sesión; Super+Alt+S lo activa o desactiva"),
	B("access.onscreen_keyboard", "Teclado en pantalla al iniciar sesión; Super+Alt+K lo muestra u oculta"),

	/* ---- seguridad (las aplica rros-apply como root; ver docs/SEGURIDAD.md) ---- */
	B("security.firewall", "Cortafuegos: bloquea las conexiones entrantes no pedidas"),
	B("security.firewall_ping", "Responder a ping con el cortafuegos activo"),
	S("security.firewall_allow", "Puertos abiertos, p. ej. 8080/tcp,5000-5010/udp (vacío = ninguno)"),
	B("security.kernel_hardening", "Endurecer el kernel (punteros, dmesg, BPF, ptrace, redirecciones)"),
	B("security.usb_storage", "Permitir memorias y discos USB"),
	B("security.camera", "Permitir cámaras web"),
	I("security.login_attempts", 0, 20, "Intentos de contraseña antes de bloquear 10 minutos (0 = sin límite)"),
	I("security.sudo_timeout_minutes", 0, 60, "Minutos que sudo recuerda tu contraseña (0 = la pide siempre)"),
	B("security.core_dumps", "Guardar volcados de memoria de programas que fallan"),
	/* antivirus (ClamAV) */
	B("security.av_realtime", "Antivirus: protección en tiempo real (analiza cada archivo de /home al abrirlo; usa ~1 GB de RAM)"),
	E("security.av_schedule", "off|diario|semanal", "Antivirus: análisis completo programado"),
	E("security.av_action", "cuarentena|avisar", "Antivirus: qué hacer con una amenaza"),
	B("security.av_updates", "Antivirus: descargar las firmas nuevas solo (cada hora)"),
	I("security.av_max_file_mb", 5, 4000, "Antivirus: tamaño máximo de archivo que se analiza (MB)"),
	/* directivas */
	I("security.password_min_length", 0, 64, "Directiva: longitud mínima de las contraseñas nuevas (0 = sin mínimo)"),
	B("security.password_complex", "Directiva: las contraseñas nuevas deben mezclar mayúsculas, minúsculas, números y símbolos"),
	B("security.usb_guard", "Directiva: bloquear los dispositivos USB nuevos hasta que los autorices"),
	B("security.tmp_noexec", "Directiva: no ejecutar programas desde /tmp"),

	/* ---- arranque ---- */
	I("boot.timeout_seconds", 0, 60, "Tiempo del menú de arranque"),
	B("boot.splash", "Pantalla de arranque gráfica"),
	B("boot.quiet", "Arranque silencioso"),
};

const size_t rros_schema_len = sizeof rros_schema / sizeof rros_schema[0];

const struct rros_key *rros_schema_find(const char *key)
{
	for (size_t i = 0; i < rros_schema_len; i++)
		if (strcmp(rros_schema[i].key, key) == 0)
			return &rros_schema[i];
	return NULL;
}

int rros_locale_name_ok(const char *v)
{
	/* ll_CC.UTF-8 (es_ES.UTF-8, ast_ES.UTF-8) o C.UTF-8 */
	size_t n = strlen(v), i = 0;

	if (!strcmp(v, "C.UTF-8"))
		return 1;
	while (i < n && v[i] >= 'a' && v[i] <= 'z')
		i++;
	if (i < 2 || i > 3 || v[i] != '_')
		return 0;
	if (!(v[i + 1] >= 'A' && v[i + 1] <= 'Z' && v[i + 2] >= 'A' && v[i + 2] <= 'Z'))
		return 0;
	return !strcmp(v + i + 3, ".UTF-8");
}

int rros_timezone_name_ok(const char *v)
{
	/* Region/Ciudad[/Sub] con letras, cifras, _ + -; sin "..", sin rutas absolutas */
	size_t n = strlen(v);

	if (n == 0 || n > 40 || v[0] == '/' || strstr(v, "..") || v[n - 1] == '/')
		return 0;
	for (const char *p = v; *p; p++)
		if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') ||
		      *p == '_' || *p == '+' || *p == '-' || *p == '/'))
			return 0;
	return 1;
}

int rros_key_system_scope(const char *key)
{
	static const char *pre[] = { "system.", "services.", "net.", "boot.", "security.", "locale." };

	for (size_t i = 0; i < sizeof pre / sizeof *pre; i++)
		if (!strncmp(key, pre[i], strlen(pre[i])))
			return 1;
	return 0;
}

const char *rros_type_name(enum rros_type t)
{
	switch (t) {
	case RROS_T_STR:  return "texto";
	case RROS_T_INT:  return "entero";
	case RROS_T_BOOL: return "booleano";
	default:          return "opción";
	}
}

static int in_choices(const char *choices, const char *val)
{
	size_t vl = strlen(val);
	const char *p = choices;

	while (*p) {
		const char *bar = strchr(p, '|');
		size_t n = bar ? (size_t)(bar - p) : strlen(p);

		if (n == vl && strncmp(p, val, n) == 0)
			return 1;
		if (!bar)
			break;
		p = bar + 1;
	}
	return 0;
}

int rros_schema_check(const struct rros_key *k, const char *val, int quoted,
		      char *err, size_t n)
{
	char *end;
	long x;

	switch (k->type) {
	case RROS_T_INT:
		if (quoted || !*val) {
			snprintf(err, n, "%s: se esperaba un entero sin comillas", k->key);
			return -1;
		}
		x = strtol(val, &end, 10);
		if (*end) {
			snprintf(err, n, "%s: '%s' no es un entero", k->key, val);
			return -1;
		}
		if (x < k->min || x > k->max) {
			snprintf(err, n, "%s: %ld fuera de rango [%ld, %ld]", k->key, x, k->min, k->max);
			return -1;
		}
		return 0;
	case RROS_T_BOOL:
		if (quoted || (strcmp(val, "true") && strcmp(val, "false"))) {
			snprintf(err, n, "%s: se esperaba true o false", k->key);
			return -1;
		}
		return 0;
	case RROS_T_ENUM:
		if (!in_choices(k->choices, val)) {
			snprintf(err, n, "%s: '%s' no válido (opciones: %s)", k->key, val, k->choices);
			return -1;
		}
		return 0;
	default:
		if (!strcmp(k->key, "locale.language") && !rros_locale_name_ok(val)) {
			snprintf(err, n, "%s: usa un idioma como es_ES.UTF-8, en_US.UTF-8 o C.UTF-8", k->key);
			return -1;
		}
		if (!strcmp(k->key, "locale.timezone") && !rros_timezone_name_ok(val)) {
			snprintf(err, n, "%s: usa una zona como Europe/Madrid, America/Mexico_City o UTC", k->key);
			return -1;
		}
		if (!strcmp(k->key, "security.firewall_allow")) {
			char why[160];

			if (rros_fw_ports_check(val, why, sizeof why)) {
				snprintf(err, n, "%s: %s", k->key, why);
				return -1;
			}
		}
		if (strlen(val) > 256) {
			snprintf(err, n, "%s: valor demasiado largo (máx. 256)", k->key);
			return -1;
		}
		for (const char *p = val; *p; p++)
			if ((unsigned char)*p < 0x20 && *p != '\t') {
				snprintf(err, n, "%s: caracteres de control no permitidos", k->key);
				return -1;
			}
		return 0;
	}
}


/* ---------------- claves dinámicas ---------------- */

enum rros_family rros_family_of(const char *key)
{
	if (!strncmp(key, "keybind.", 8))
		return RROS_FAM_KEYBIND;
	if (!strncmp(key, "rule.", 5))
		return RROS_FAM_RULE;
	if (!strncmp(key, "autostart.", 10))
		return RROS_FAM_AUTOSTART;
	return RROS_FAM_NONE;
}

const char *rros_family_desc(enum rros_family f)
{
	switch (f) {
	case RROS_FAM_KEYBIND:   return "Atajo: \"COMBINACION COMANDO\", p. ej. \"Mod+Shift+b firefox\"";
	case RROS_FAM_RULE:      return "Regla de ventana: \"app=NOMBRE ACCION\" (above, fullscreen, maximized, noborder, "
				        "skiptaskbar, minimized u opacity N), p. ej. \"app=mpv above\"";
	case RROS_FAM_AUTOSTART: return "Programa que se inicia con el escritorio";
	default:                 return "";
	}
}

static int id_ok(const char *id)
{
	size_t n = strlen(id);

	if (n == 0 || n > 32)
		return 0;
	for (size_t i = 0; i < n; i++)
		if (!((id[i] >= 'a' && id[i] <= 'z') || (id[i] >= '0' && id[i] <= '9') ||
		      id[i] == '_' || id[i] == '-'))
			return 0;
	return 1;
}

static const char *fam_id(const char *key)
{
	const char *dot = strchr(key, '.');

	return dot ? dot + 1 : key;
}

static int text_ok(const char *s, size_t max)
{
	size_t n = strlen(s);

	if (n == 0 || n > max)
		return 0;
	for (; *s; s++)
		if ((unsigned char)*s < 0x20 || *s == '{' || *s == '}')
			return 0;
	return 1;
}

/* Copia el primer token (hasta espacio) en tok y devuelve el resto sin espacios iniciales. */
static const char *next_token(const char *s, char *tok, size_t n)
{
	size_t i = 0;

	while (*s == ' ')
		s++;
	while (*s && *s != ' ' && i + 1 < n)
		tok[i++] = *s++;
	tok[i] = '\0';
	while (*s == ' ')
		s++;
	return s;
}

/* "Mod+Shift+b firefox" (también con "exec" delante, como antes) -> atajo de KDE "Meta+Shift+B" y comando */
int rros_keybind_parse(const char *val, char *combo, size_t cn, char *cmd, size_t mn)
{
	char raw[96], key[32] = "";
	const char *rest, *p;
	int mods[4] = { 0 };   /* Meta, Ctrl, Alt, Shift: KDE los escribe en ese orden */
	size_t o = 0;

	if (!text_ok(val, 200))
		return -1;
	rest = next_token(val, raw, sizeof raw);
	if (!strncmp(rest, "exec ", 5))
		rest += 5;
	while (*rest == ' ')
		rest++;
	if (!raw[0] || !*rest)
		return -1;
	p = raw;
	while (*p) {
		char part[32];
		size_t pl = 0;

		while (*p && *p != '+' && pl + 1 < sizeof part) {
			if (!(isalnum((unsigned char)*p) || *p == '_'))
				return -1;
			part[pl++] = *p++;
		}
		part[pl] = '\0';
		if (pl == 0 || (*p && *p != '+'))
			return -1;
		if (*p == '+' && !*++p)
			return -1;
		if (key[0])
			return -1;   /* la tecla va al final */
		if (!strcmp(part, "Mod") || !strcmp(part, "Mod4") || !strcasecmp(part, "Super") || !strcasecmp(part, "Meta"))
			mods[0] = 1;
		else if (!strcasecmp(part, "Ctrl") || !strcasecmp(part, "Control"))
			mods[1] = 1;
		else if (!strcasecmp(part, "Alt") || !strcmp(part, "Mod1"))
			mods[2] = 1;
		else if (!strcasecmp(part, "Shift"))
			mods[3] = 1;
		else if (*p)
			return -1;   /* modificador desconocido */
		else
			snprintf(key, sizeof key, "%s", part);
	}
	if (!key[0] || cn < 2)
		return -1;
	{
		static const char *names[] = { "Meta", "Ctrl", "Alt", "Shift" };

		combo[0] = '\0';
		for (int i = 0; i < 4; i++)
			if (mods[i])
				o += (size_t)snprintf(combo + o, o < cn ? cn - o : 0, "%s+", names[i]);
		if (o >= cn)
			return -1;
		/* una letra suelta va en mayúscula; Return, space, F1... con la primera en mayúscula */
		if ((size_t)snprintf(combo + o, cn - o, "%c%s", toupper((unsigned char)key[0]), key + 1) >= cn - o)
			return -1;
	}
	if ((size_t)snprintf(cmd, mn, "%s", rest) >= mn)
		return -1;
	return 0;
}

/* "app=firefox above" -> regla de KWin. Criterios: app (también app_id y class) o title. */
int rros_rule_parse(const char *val, struct rros_rule *r)
{
	char crit[96], act[32], num[32];
	const char *rest, *eq;

	memset(r, 0, sizeof *r);
	if (!text_ok(val, 200))
		return -1;
	rest = next_token(val, crit, sizeof crit);
	eq = strchr(crit, '=');
	if (!eq || !*rest)
		return -1;
	if (!strncmp(crit, "app=", 4) || !strncmp(crit, "app_id=", 7) || !strncmp(crit, "class=", 6))
		r->by_title = 0;
	else if (!strncmp(crit, "title=", 6))
		r->by_title = 1;
	else
		return -1;
	snprintf(r->match, sizeof r->match, "%s", eq + 1);
	if (!r->match[0])
		return -1;
	for (const char *c = r->match; *c; c++)
		if (!(isalnum((unsigned char)*c) || *c == '.' || *c == '_' || *c == '-'))
			return -1;
	rest = next_token(rest, act, sizeof act);
	rest = next_token(rest, num, sizeof num);
	if (*rest)
		return -1;
	if (!strcmp(act, "opacity")) {
		char *end;
		double x = strtod(num, &end);

		if (!num[0] || *end || x <= 0 || x > 100)
			return -1;
		r->opacity = x <= 1.0 ? (int)(x * 100 + .5) : (int)x;
		if (r->opacity < 10)
			return -1;
		snprintf(r->action, sizeof r->action, "opacity");
		return 0;
	}
	if (num[0])
		return -1;
	/* «floating» e «inhibit_idle» venían de sway: en Plasma las ventanas ya son libres */
	if (strcmp(act, "above") && strcmp(act, "fullscreen") && strcmp(act, "maximized") && strcmp(act, "noborder") &&
	    strcmp(act, "skiptaskbar") && strcmp(act, "minimized"))
		return -1;
	snprintf(r->action, sizeof r->action, "%s", act);
	return 0;
}

static int autostart_ok(const char *val)
{
	return text_ok(val, 200) ? 0 : -1;
}

int rros_dynamic_check(const char *key, const char *val, char *err, size_t n)
{
	enum rros_family f = rros_family_of(key);
	char tmp[512];
	int rc;

	if (f == RROS_FAM_NONE)
		return 1;
	if (!id_ok(fam_id(key))) {
		snprintf(err, n, "%s: el identificador debe ser a-z, 0-9, _ o - (máx. 32)", key);
		return -1;
	}
	if (f == RROS_FAM_KEYBIND) {
		char cmd[256];

		rc = rros_keybind_parse(val, tmp, sizeof tmp, cmd, sizeof cmd);
	} else if (f == RROS_FAM_RULE) {
		struct rros_rule r;

		rc = rros_rule_parse(val, &r);
	} else {
		rc = autostart_ok(val);
	}
	if (rc) {
		snprintf(err, n, "%s: valor no válido. %s", key, rros_family_desc(f));
		return -1;
	}
	return 0;
}
