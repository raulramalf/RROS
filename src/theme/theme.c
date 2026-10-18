#include "theme.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "rros_profile.h"
#include "rros_schema.h"

static int valid_hex(const char *s)
{
	if (strlen(s) != 7 || s[0] != '#')
		return 0;
	for (int i = 1; i < 7; i++)
		if (!isxdigit((unsigned char)s[i]))
			return 0;
	return 1;
}

static void rgb_of(const char *h, int *r, int *g, int *b)
{
	unsigned v = (unsigned)strtoul(h + 1, NULL, 16);

	*r = (int)((v >> 16) & 255);
	*g = (int)((v >> 8) & 255);
	*b = (int)(v & 255);
}

static int safe_chars(const char *s, const char *extra)
{
	for (; *s; s++)
		if (!isalnum((unsigned char)*s) && !strchr(extra, *s))
			return 0;
	return 1;
}

static long cfg_int(const struct rros_conf *c, const char *key, long def)
{
	long v = rros_conf_get_long(c, key, def);
	const struct rros_key *k = rros_schema_find(key);

	if (k && k->type == RROS_T_INT) {
		if (v < k->min)
			v = k->min;
		if (v > k->max)
			v = k->max;
	}
	return v;
}

static void cfg_str(const struct rros_conf *c, const char *key, const char *def,
		    const char *extra, char *dst, size_t n)
{
	const char *v = rros_conf_get(c, key);

	if (!v || !*v || !safe_chars(v, extra) || strlen(v) >= n)
		v = def;
	snprintf(dst, n, "%s", v);
}

static void cfg_enum(const struct rros_conf *c, const char *key, const char *def, char *dst, size_t n)
{
	const char *v = rros_conf_get(c, key);
	const struct rros_key *k = rros_schema_find(key);
	char err[128];

	if (!v || !k || rros_schema_check(k, v, 1, err, sizeof err) || strlen(v) >= n)
		v = def;
	snprintf(dst, n, "%s", v);
}

void rros_theme_from_conf(struct rros_theme *t, const struct rros_conf *c)
{
	const char *acc = rros_conf_get(c, "ui.accent");
	char scheme[8], tmp[12];

	memset(t, 0, sizeof *t);
	if (acc && valid_hex(acc)) {
		snprintf(t->accent, sizeof t->accent, "%s", acc);
		for (char *p = t->accent; *p; p++)
			*p = (char)tolower((unsigned char)*p);
	}
	cfg_str(c, "ui.theme", "aurora", "_-", t->theme, sizeof t->theme);
	if (!rros_valid_profile_name(t->theme))
		snprintf(t->theme, sizeof t->theme, "aurora");
	cfg_enum(c, "ui.color_scheme", "dark", scheme, sizeof scheme);
	t->dark = strcmp(scheme, "light") != 0;
	cfg_str(c, "ui.font", "Inter", " ._-", t->font, sizeof t->font);
	cfg_str(c, "ui.font_mono", "JetBrains Mono", " ._-", t->font_mono, sizeof t->font_mono);
	cfg_str(c, "ui.icon_theme", "breeze", "._-", t->icon_theme, sizeof t->icon_theme);
	cfg_str(c, "ui.cursor_theme", "breeze_cursors", "._-", t->cursor_theme, sizeof t->cursor_theme);
	cfg_enum(c, "ui.font_hinting", "slight", t->font_hinting, sizeof t->font_hinting);
	cfg_enum(c, "ui.font_antialias", "rgba", t->font_aa, sizeof t->font_aa);
	t->cursor_size = (int)cfg_int(c, "ui.cursor_size", 24);
	t->font_size = (int)cfg_int(c, "ui.font_size", 10);
	t->scale = (int)cfg_int(c, "ui.scale", 100);
	t->radius = (int)cfg_int(c, "ui.corner_radius", 10);
	t->panel_size = (int)cfg_int(c, "ui.panel_size", 28);
	t->workspaces = (int)cfg_int(c, "ui.workspaces", 1);
	t->transparency = (int)cfg_int(c, "ui.transparency", 0);
	cfg_enum(c, "ui.panel_position", "top", t->panel_pos, sizeof t->panel_pos);
	cfg_enum(c, "panel.style", "flat", t->panel_style, sizeof t->panel_style);
	cfg_enum(c, "ui.effects", "basic", t->effects, sizeof t->effects);
	cfg_enum(c, "ui.wallpaper_mode", "fill", t->wallpaper_mode, sizeof t->wallpaper_mode);
	cfg_str(c, "input.keymap", "es", "_,", t->keymap, sizeof t->keymap);
	cfg_str(c, "apps.terminal", "konsole", "._- /", t->app_term, sizeof t->app_term);
	cfg_str(c, "apps.browser", "firefox", "._- /", t->app_browser, sizeof t->app_browser);
	cfg_str(c, "apps.files", "dolphin", "._- /", t->app_files, sizeof t->app_files);
	cfg_str(c, "apps.editor", "kate", "._- /", t->app_editor, sizeof t->app_editor);
	t->lock_min = (int)cfg_int(c, "privacy.lock_minutes", 10);
	t->screen_off_min = (int)cfg_int(c, "power.screen_off_minutes", 15);
	t->suspend_min = (int)cfg_int(c, "power.suspend_minutes", 30);
	cfg_enum(c, "power.lid_action", "suspend", t->lid_action, sizeof t->lid_action);
	cfg_str(c, "ui.wallpaper", "", "/._-", t->wallpaper, sizeof t->wallpaper);
	t->term_scrollback = (int)cfg_int(c, "term.scrollback", 5000);
	t->term_padding = (int)cfg_int(c, "term.padding", 10);
	cfg_enum(c, "term.cursor", "block", t->term_cursor, sizeof t->term_cursor);
	t->animations = rros_conf_get_bool(c, "ui.animations", 1);
	t->high_contrast = rros_conf_get_bool(c, "ui.high_contrast", 0);
	t->reduce_motion = rros_conf_get_bool(c, "ui.reduce_motion", 0);
	t->screen_reader = rros_conf_get_bool(c, "access.screen_reader", 0);
	t->osk = rros_conf_get_bool(c, "access.onscreen_keyboard", 0);
	t->taskbar = rros_conf_get_bool(c, "panel.taskbar", 0);
	if (t->reduce_motion)
		t->animations = 0;
	t->blur = rros_conf_get_bool(c, "ui.blur", 0);
	t->shadows = rros_conf_get_bool(c, "ui.shadows", 1);
	t->clock_24h = rros_conf_get_bool(c, "ui.clock_24h", 1);
	t->battery_percent = rros_conf_get_bool(c, "ui.show_battery_percent", 1);
	t->panel_autohide = rros_conf_get_bool(c, "ui.panel_autohide", 0);
	t->natural_scroll = rros_conf_get_bool(c, "input.natural_scroll", 1);
	t->tap_to_click = rros_conf_get_bool(c, "input.tap_to_click", 1);
	t->dwt = rros_conf_get_bool(c, "input.disable_while_typing", 1);
	t->numlock = rros_conf_get_bool(c, "input.numlock_on_boot", 1);
	t->pointer_speed = (int)cfg_int(c, "input.pointer_speed", 0);
	t->repeat_delay = (int)cfg_int(c, "input.repeat_delay_ms", 300);
	t->repeat_rate = (int)cfg_int(c, "input.repeat_rate", 30);
	t->battery_low = (int)cfg_int(c, "power.battery_low_percent", 15);
	t->animation_speed = (int)cfg_int(c, "ui.animation_speed", 100);
	cfg_enum(c, "ui.animation_style", "suave", t->animation_style, sizeof t->animation_style);
	t->blur_strength = (int)cfg_int(c, "ui.blur_strength", 6);
	t->dim_inactive = rros_conf_get_bool(c, "ui.dim_inactive", 0);
	t->hot_corners = rros_conf_get_bool(c, "ui.hot_corners", 1);
	t->dock = rros_conf_get_bool(c, "ui.dock", 1);
	cfg_enum(c, "ui.dock_style", "2d", tmp, sizeof tmp);
	t->dock_3d = !strcmp(tmp, "3d");
	t->dock_icon = (int)cfg_int(c, "ui.dock_icon_size", 48);
	cfg_enum(c, "ui.dock_position", "bottom", t->dock_pos, sizeof t->dock_pos);
	cfg_enum(c, "ui.dock_hiding", "dodge", t->dock_hiding, sizeof t->dock_hiding);
	t->dock_launchpad = rros_conf_get_bool(c, "ui.dock_launchpad", 1);
	cfg_enum(c, "ui.window_buttons", "left", tmp, sizeof tmp);
	t->buttons_left = strcmp(tmp, "right") != 0;
	t->global_menu = rros_conf_get_bool(c, "ui.global_menu", 1);
	t->gestures = rros_conf_get_bool(c, "input.gestures", 1);
	cfg_enum(c, "ui.wallpaper_style", "aurora", t->wallpaper_style, sizeof t->wallpaper_style);
	cfg_enum(c, "display.vrr", "auto", t->vrr, sizeof t->vrr);
	cfg_enum(c, "ui.night_light", "off", t->night_light, sizeof t->night_light);
	t->night_temp = (int)cfg_int(c, "ui.night_light_temp", 4500);
	cfg_enum(c, "audio.sound_theme", "ocean", t->sound_theme, sizeof t->sound_theme);
	t->event_sounds = rros_conf_get_bool(c, "audio.event_sounds", 1);
	t->max_volume = (int)cfg_int(c, "audio.max_volume", 100);
	t->volume_step = (int)cfg_int(c, "audio.volume_step", 5);
	t->volume_feedback = rros_conf_get_bool(c, "audio.volume_feedback", 1);
	t->indexer = rros_conf_get_bool(c, "services.indexer", 0);
}

void rros_hex_rgb(const char *hex, int *r, int *g, int *b)
{
	rgb_of(hex, r, g, b);
}

/* ---- estilos de escritorio ---- */

const struct rros_desktop_preset rros_desktop_presets[] = {
	{ "moderno", "Moderno", "El de RROS, como en macOS: barra fina arriba con el menú RROS, el menú de la aplicación, "
	  "la bandeja y el reloj, y un dock abajo con tus aplicaciones y el Launchpad.",
	  { { "ui.panel_position", "top" }, { "panel.style", "flat" }, { "ui.panel_size", "28" },
	    { "panel.taskbar", "false" }, { "ui.dock", "true" }, { "ui.dock_position", "bottom" },
	    { "ui.dock_hiding", "dodge" }, { "ui.global_menu", "true" }, { "ui.panel_autohide", "false" } } },
	{ "clasico", "Clásico", "Como en Windows: barra de tareas abajo con el botón de aplicaciones, las ventanas abiertas, "
	  "la bandeja y el reloj. Sin dock.",
	  { { "ui.panel_position", "bottom" }, { "panel.style", "flat" }, { "ui.panel_size", "44" },
	    { "panel.taskbar", "true" }, { "ui.dock", "false" }, { "ui.global_menu", "false" },
	    { "ui.panel_autohide", "false" } } },
	{ "enfoque", "Enfoque", "Para concentrarse: barra en píldora arriba con el reloj en el centro y el dock escondido "
	  "hasta que llevas el ratón abajo. El escritorio, limpio del todo.",
	  { { "ui.panel_position", "top" }, { "panel.style", "pill" }, { "ui.panel_size", "28" },
	    { "panel.taskbar", "false" }, { "ui.dock", "true" }, { "ui.dock_position", "bottom" },
	    { "ui.dock_hiding", "auto" }, { "ui.global_menu", "true" }, { "ui.panel_autohide", "false" } } },
};
const size_t rros_desktop_presets_len = sizeof rros_desktop_presets / sizeof rros_desktop_presets[0];

const struct rros_desktop_preset *rros_desktop_find(const char *id)
{
	for (size_t i = 0; i < rros_desktop_presets_len; i++)
		if (!strcmp(rros_desktop_presets[i].id, id))
			return &rros_desktop_presets[i];
	return NULL;
}

const struct rros_desktop_preset *rros_desktop_current(const struct rros_conf *c)
{
	for (size_t i = 0; i < rros_desktop_presets_len; i++) {
		const struct rros_desktop_preset *d = &rros_desktop_presets[i];
		int all = 1;

		for (size_t k = 0; k < 14 && d->keys[k][0] && all; k++) {
			const char *v = rros_conf_get(c, d->keys[k][0]);

			all = v && !strcmp(v, d->keys[k][1]);
		}
		if (all)
			return d;
	}
	return NULL;
}
