#include "rros_schema.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

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
	E("ui.compositor", "minimal|full", "Compositor Wayland"),
	E("ui.effects", "off|basic|advanced", "Nivel de efectos visuales"),
	B("ui.animations", "Animaciones de ventanas y menús"),
	I("ui.animation_speed", 25, 400, "Velocidad de animación (%)"),
	I("ui.transparency", 0, 100, "Transparencia de paneles y ventanas (%)"),
	B("ui.blur", "Desenfoque de fondo"),
	B("ui.shadows", "Sombras de ventanas"),
	I("ui.corner_radius", 0, 32, "Radio de esquinas (px)"),
	I("ui.gap", 0, 64, "Separación entre ventanas (px)"),
	I("ui.border_width", 0, 16, "Grosor de borde de ventana (px)"),
	E("ui.layout", "floating|tiling|hybrid", "Modo de gestión de ventanas"),
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

	/* ---- panel (waybar) ---- */
	E("panel.style", "flat|floating|pill", "Estilo del panel"),
	S("panel.left", "Módulos a la izquierda, separados por comas"),
	S("panel.center", "Módulos en el centro, separados por comas"),
	S("panel.right", "Módulos a la derecha, separados por comas"),

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
	B("services.firewall", "Cortafuegos"),
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
	B("audio.event_sounds", "Sonidos de eventos"),

	/* ---- privacidad ---- */
	B("privacy.telemetry", "Telemetría (desactivada por defecto)"),
	B("privacy.crash_reports", "Informes de fallos"),
	I("privacy.lock_minutes", 0, 1440, "Bloquear pantalla tras N minutos (0 = nunca)"),
	B("privacy.disk_encryption", "Cifrado de disco (LUKS) en la instalación"),

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
	case RROS_FAM_KEYBIND:   return "Atajo: \"COMBINACION COMANDO\", p. ej. \"Mod+Shift+b exec firefox\"";
	case RROS_FAM_RULE:      return "Regla de ventana: \"app_id=NOMBRE ACCION\", p. ej. \"app_id=firefox workspace 2\"";
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

int rros_keybind_to_sway(const char *val, char *out, size_t n)
{
	static const char *cmds[] = { "exec", "workspace", "move", "focus", "kill", "fullscreen", "floating",
				      "layout", "splith", "splitv", "reload", "resize", "scratchpad", "sticky", "mode" };
	char combo[96], cmd[32], mapped[128] = "";
	const char *rest, *p;
	size_t mi = 0;
	int ok = 0;

	if (!text_ok(val, 200))
		return -1;
	rest = next_token(val, combo, sizeof combo);
	if (!combo[0] || !*rest)
		return -1;
	/* combinación: tokens [A-Za-z0-9_] separados por '+'; "Mod" pasa a $mod */
	p = combo;
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
		if (mi && mi < sizeof mapped - 1)
			mapped[mi++] = '+';
		if (!strcmp(part, "Mod"))
			mi += (size_t)snprintf(mapped + mi, sizeof mapped - mi, "$mod");
		else
			mi += (size_t)snprintf(mapped + mi, sizeof mapped - mi, "%s", part);
		if (mi >= sizeof mapped - 1)
			return -1;
		if (*p == '+') {
			p++;
			if (!*p)
				return -1;
		}
	}
	next_token(rest, cmd, sizeof cmd);
	for (size_t i = 0; i < sizeof cmds / sizeof *cmds; i++)
		if (!strcmp(cmd, cmds[i]))
			ok = 1;
	if (!ok)
		return -1;
	if ((size_t)snprintf(out, n, "bindsym %s %s", mapped, rest) >= n)
		return -1;
	return 0;
}

int rros_rule_to_sway(const char *val, char *out, size_t n)
{
	char crit[96], act[32], num[32];
	const char *rest, *eq;
	char key[16], value[64], who[160];
	int w;

	if (!text_ok(val, 200))
		return -1;
	rest = next_token(val, crit, sizeof crit);
	eq = strchr(crit, '=');
	if (!eq || !*rest)
		return -1;
	if ((size_t)(eq - crit) >= sizeof key)
		return -1;
	memcpy(key, crit, (size_t)(eq - crit));
	key[eq - crit] = '\0';
	if (strcmp(key, "app_id") && strcmp(key, "title") && strcmp(key, "class") && strcmp(key, "window_role"))
		return -1;
	snprintf(value, sizeof value, "%s", eq + 1);
	if (!value[0])
		return -1;
	for (const char *c = value; *c; c++)
		if (!(isalnum((unsigned char)*c) || *c == '.' || *c == '_' || *c == '-'))
			return -1;
	snprintf(who, sizeof who, "[%s=\"%s\"]", key, value);
	rest = next_token(rest, act, sizeof act);

	if (!strcmp(act, "floating") && !*rest)
		w = snprintf(out, n, "for_window %s floating enable", who);
	else if (!strcmp(act, "sticky") && !*rest)
		w = snprintf(out, n, "for_window %s sticky enable", who);
	else if (!strcmp(act, "fullscreen") && !*rest)
		w = snprintf(out, n, "for_window %s fullscreen enable", who);
	else if (!strcmp(act, "inhibit_idle") && !*rest)
		w = snprintf(out, n, "for_window %s inhibit_idle fullscreen", who);
	else if (!strcmp(act, "workspace")) {
		char *end;
		long x;

		next_token(rest, num, sizeof num);
		x = strtol(num, &end, 10);
		if (!num[0] || *end || x < 1 || x > 32)
			return -1;
		w = snprintf(out, n, "assign %s workspace number %ld", who, x);
	} else if (!strcmp(act, "opacity")) {
		char *end;
		double x;

		next_token(rest, num, sizeof num);
		x = strtod(num, &end);
		if (!num[0] || *end || x < 0.0 || x > 1.0)
			return -1;
		w = snprintf(out, n, "for_window %s opacity %.2f", who, x);
	} else if (!strcmp(act, "border")) {
		char kind[16];
		const char *r2 = next_token(rest, kind, sizeof kind);

		if (!strcmp(kind, "none") && !*r2) {
			w = snprintf(out, n, "for_window %s border none", who);
		} else if (!strcmp(kind, "pixel")) {
			char *end;
			long x;

			next_token(r2, num, sizeof num);
			x = strtol(num, &end, 10);
			if (!num[0] || *end || x < 0 || x > 16)
				return -1;
			w = snprintf(out, n, "for_window %s border pixel %ld", who, x);
		} else {
			return -1;
		}
	} else {
		return -1;
	}
	return (w < 0 || (size_t)w >= n) ? -1 : 0;
}

int rros_autostart_to_sway(const char *val, char *out, size_t n)
{
	if (!text_ok(val, 200))
		return -1;
	return (size_t)snprintf(out, n, "exec %s", val) >= n ? -1 : 0;
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
	rc = f == RROS_FAM_KEYBIND ? rros_keybind_to_sway(val, tmp, sizeof tmp) :
	     f == RROS_FAM_RULE ? rros_rule_to_sway(val, tmp, sizeof tmp) :
	     rros_autostart_to_sway(val, tmp, sizeof tmp);
	if (rc) {
		snprintf(err, n, "%s: valor no válido. %s", key, rros_family_desc(f));
		return -1;
	}
	return 0;
}
