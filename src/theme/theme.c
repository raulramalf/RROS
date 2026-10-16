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
	char scheme[8], comp[8];

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
	cfg_str(c, "ui.icon_theme", "Adwaita", "._-", t->icon_theme, sizeof t->icon_theme);
	cfg_str(c, "ui.cursor_theme", "Adwaita", "._-", t->cursor_theme, sizeof t->cursor_theme);
	t->cursor_size = (int)cfg_int(c, "ui.cursor_size", 24);
	t->font_size = (int)cfg_int(c, "ui.font_size", 11);
	t->scale = (int)cfg_int(c, "ui.scale", 100);
	t->gap = (int)cfg_int(c, "ui.gap", 8);
	t->border = (int)cfg_int(c, "ui.border_width", 2);
	t->radius = (int)cfg_int(c, "ui.corner_radius", 8);
	t->panel_size = (int)cfg_int(c, "ui.panel_size", 32);
	t->workspaces = (int)cfg_int(c, "ui.workspaces", 4);
	t->transparency = (int)cfg_int(c, "ui.transparency", 0);
	cfg_enum(c, "ui.panel_position", "top", t->panel_pos, sizeof t->panel_pos);
	cfg_enum(c, "panel.style", "flat", t->panel_style, sizeof t->panel_style);
	cfg_enum(c, "ui.layout", "hybrid", t->layout, sizeof t->layout);
	cfg_enum(c, "ui.effects", "basic", t->effects, sizeof t->effects);
	cfg_enum(c, "ui.wallpaper_mode", "fill", t->wallpaper_mode, sizeof t->wallpaper_mode);
	cfg_str(c, "input.keymap", "es", "_,", t->keymap, sizeof t->keymap);
	cfg_str(c, "panel.left", "apps,workspaces,mode", ",_", t->panel_left, sizeof t->panel_left);
	cfg_str(c, "panel.center", "clock", ",_", t->panel_center, sizeof t->panel_center);
	cfg_str(c, "panel.right", "profile,memory,cpu,network,audio,battery,tray,power", ",_", t->panel_right,
		sizeof t->panel_right);
	cfg_str(c, "apps.terminal", "foot", "._- /", t->app_term, sizeof t->app_term);
	cfg_str(c, "apps.browser", "epiphany", "._- /", t->app_browser, sizeof t->app_browser);
	cfg_str(c, "apps.files", "thunar", "._- /", t->app_files, sizeof t->app_files);
	cfg_str(c, "apps.editor", "mousepad", "._- /", t->app_editor, sizeof t->app_editor);
	t->lock_min = (int)cfg_int(c, "privacy.lock_minutes", 10);
	t->screen_off_min = (int)cfg_int(c, "power.screen_off_minutes", 15);
	t->suspend_min = (int)cfg_int(c, "power.suspend_minutes", 30);
	cfg_str(c, "ui.wallpaper", "", "/._-", t->wallpaper, sizeof t->wallpaper);
	t->term_scrollback = (int)cfg_int(c, "term.scrollback", 5000);
	t->term_padding = (int)cfg_int(c, "term.padding", 10);
	cfg_enum(c, "term.cursor", "block", t->term_cursor, sizeof t->term_cursor);
	t->animations = rros_conf_get_bool(c, "ui.animations", 1);
	t->blur = rros_conf_get_bool(c, "ui.blur", 0);
	t->shadows = rros_conf_get_bool(c, "ui.shadows", 0);
	t->clock_24h = rros_conf_get_bool(c, "ui.clock_24h", 1);
	t->battery_percent = rros_conf_get_bool(c, "ui.show_battery_percent", 1);
	t->natural_scroll = rros_conf_get_bool(c, "input.natural_scroll", 1);
	t->tap_to_click = rros_conf_get_bool(c, "input.tap_to_click", 1);
	t->dwt = rros_conf_get_bool(c, "input.disable_while_typing", 1);
	t->numlock = rros_conf_get_bool(c, "input.numlock_on_boot", 1);
	t->pointer_speed = (int)cfg_int(c, "input.pointer_speed", 0);
	t->repeat_delay = (int)cfg_int(c, "input.repeat_delay_ms", 300);
	t->repeat_rate = (int)cfg_int(c, "input.repeat_rate", 30);
	t->battery_low = (int)cfg_int(c, "power.battery_low_percent", 15);
	cfg_enum(c, "ui.compositor", "minimal", comp, sizeof comp);
	t->full_compositor = strcmp(comp, "full") == 0;
}

/* ---- utilidades de generación ---- */

static const char *nh(const char *hex)
{
	return hex + 1;
}

static double alpha_of(const struct rros_theme *t, double min)
{
	double a = 1.0 - t->transparency / 100.0;

	return a < min ? min : a;
}

static void hex_alpha(char *out, size_t n, const char *hex, double alpha, int hash)
{
	snprintf(out, n, "%s%s%02x", hash ? "#" : "", nh(hex), (unsigned)(alpha * 255.0 + 0.5));
}

static int by_key(const void *a, const void *b)
{
	const struct rros_kv *x = *(const struct rros_kv *const *)a, *y = *(const struct rros_kv *const *)b;

	return strcmp(x->key, y->key);
}

/* Reúne las claves que empiezan por prefix, ordenadas, para una salida estable. */
static size_t collect(const struct rros_conf *c, const char *prefix, const struct rros_kv **out, size_t max)
{
	size_t n = 0, pl = strlen(prefix);

	for (size_t i = 0; i < c->n && n < max; i++)
		if (!strncmp(c->items[i].key, prefix, pl))
			out[n++] = &c->items[i];
	if (n > 1)
		qsort(out, n, sizeof *out, by_key);
	return n;
}

/* ---- sway ---- */

void rros_render_sway(const struct rros_theme *t, const struct rros_palette *p,
		      const struct rros_conf *c, FILE *f)
{
	const struct rros_kv *items[64];
	char line[600];
	size_t n;

	fprintf(f, "# %s. No lo edites: usa `rros-config set ui.<clave> <valor>`.\n", RROS_GENERATED);
	fprintf(f, "set $mod Mod4\nset $term %s\nset $menu fuzzel\n\n", t->app_term);
	fprintf(f, "font pango:%s %d\n", t->font, t->font_size);
	fprintf(f, "default_border pixel %d\ndefault_floating_border pixel %d\n", t->border, t->border);
	fprintf(f, "gaps inner %d\ngaps outer %d\nsmart_borders on\n", t->gap, t->gap / 2);
	fputs("focus_follows_mouse yes\nfloating_modifier $mod normal\n", f);
	fprintf(f, "seat seat0 xcursor_theme %s %d\n\n", t->cursor_theme, t->cursor_size);
	fprintf(f, "client.focused          %s %s %s %s %s\n", p->accent, p->accent, p->bg, p->accent, p->accent);
	fprintf(f, "client.focused_inactive %s %s %s %s %s\n", p->border, p->surface, p->muted, p->border, p->border);
	fprintf(f, "client.unfocused        %s %s %s %s %s\n", p->border, p->surface, p->muted, p->border, p->border);
	fprintf(f, "client.urgent           %s %s %s %s %s\n", p->red, p->red, p->bg, p->red, p->red);
	fprintf(f, "client.background       %s\n", p->bg);
	if (t->wallpaper[0] && access(t->wallpaper, R_OK) == 0)
		fprintf(f, "output * bg %s %s\n", t->wallpaper, t->wallpaper_mode);
	else
		fprintf(f, "output * bg %s solid_color\n", p->bg);
	if (t->scale != 100)
		fprintf(f, "output * scale %.2f\n", t->scale / 100.0);

	fputs("\ninput type:keyboard {\n", f);
	fprintf(f, "    xkb_layout %s\n    repeat_delay %d\n    repeat_rate %d\n", t->keymap,
		t->repeat_delay, t->repeat_rate);
	fprintf(f, "    xkb_numlock %s\n}\n", t->numlock ? "enabled" : "disabled");
	fprintf(f, "input type:touchpad {\n    natural_scroll %s\n    tap %s\n    dwt %s\n    pointer_accel %.2f\n}\n",
		t->natural_scroll ? "enabled" : "disabled", t->tap_to_click ? "enabled" : "disabled",
		t->dwt ? "enabled" : "disabled", t->pointer_speed / 100.0);
	fprintf(f, "input type:pointer {\n    pointer_accel %.2f\n}\n", t->pointer_speed / 100.0);

	if (strcmp(t->layout, "floating") == 0)
		fputs("\nfor_window [app_id=\".*\"] floating enable\nfor_window [class=\".*\"] floating enable\n", f);
	else if (strcmp(t->layout, "hybrid") == 0)
		fputs("\nworkspace_layout tabbed\n", f);

	/* Atajos de RROS. Si el usuario define uno con la misma combinación, el suyo sustituye al de RROS:
	 * sway trata una combinación repetida como error y mostraría la barra roja. */
	{
		const struct rros_kv *ukb[64];
		char over[64][96];
		size_t nover = collect(c, "keybind.", ukb, 64), nov = 0;
		char tmp[600];

		for (size_t i = 0; i < nover; i++) {
			char *sp;

			if (rros_keybind_to_sway(ukb[i]->val, tmp, sizeof tmp))
				continue;
			sp = strchr(tmp + 8, ' ');          /* "bindsym COMBO comando" */
			if (sp && (size_t)(sp - (tmp + 8)) < sizeof over[0]) {
				memcpy(over[nov], tmp + 8, (size_t)(sp - (tmp + 8)));
				over[nov][sp - (tmp + 8)] = '\0';
				nov++;
			}
		}
#define BIND(combo, ...) do { \
		int skip = 0; \
		for (size_t q = 0; q < nov; q++) \
			if (strcasecmp(over[q], combo) == 0) \
				skip = 1; \
		if (!skip) { \
			fprintf(f, "bindsym %s ", combo); \
			fprintf(f, __VA_ARGS__); \
			fputc('\n', f); \
		} \
	} while (0)
		fputs("\n# Atajos RROS\n", f);
		BIND("$mod+Return", "exec $term");
		BIND("$mod+space", "exec $menu");
		BIND("$mod+d", "exec $menu");
		BIND("$mod+q", "kill");
		BIND("$mod+f", "fullscreen");
		BIND("$mod+Shift+space", "floating toggle");
		BIND("$mod+Shift+r", "reload");
		BIND("$mod+Shift+e", "exec rros-power");
		BIND("$mod+Escape", "exec swaylock -f");
		BIND("$mod+b", "exec %s", t->app_browser);
		BIND("$mod+e", "exec %s", t->app_files);
		BIND("$mod+Shift+n", "exec %s", t->app_editor);
		BIND("$mod+v", "exec rros-clipboard");
		BIND("$mod+Shift+w", "exec nm-connection-editor");
		BIND("$mod+t", "exec rros-theme toggle");
		BIND("$mod+Shift+t", "exec rros-theme cycle");
		BIND("$mod+comma", "exec rros-settings open");
		BIND("$mod+p", "exec foot -T RROS -e sh -c 'rros-fetch; rros doctor; read -r _'");
		BIND("$mod+Left", "focus left");
		BIND("$mod+Right", "focus right");
		BIND("$mod+Up", "focus up");
		BIND("$mod+Down", "focus down");
		BIND("$mod+h", "focus left");
		BIND("$mod+l", "focus right");
		BIND("$mod+k", "focus up");
		BIND("$mod+j", "focus down");
		BIND("$mod+s", "layout stacking");
		BIND("$mod+w", "layout tabbed");
		BIND("$mod+g", "layout toggle split");
		BIND("$mod+r", "mode \"resize\"");
		BIND("Print", "exec rros-shot");
		BIND("$mod+Shift+s", "exec rros-shot region");
		BIND("XF86MonBrightnessUp", "exec brightnessctl set +5%%");
		BIND("XF86MonBrightnessDown", "exec brightnessctl set 5%%-");
		BIND("XF86AudioRaiseVolume", "exec wpctl set-volume -l 1.5 @DEFAULT_AUDIO_SINK@ 5%%+");
		BIND("XF86AudioLowerVolume", "exec wpctl set-volume @DEFAULT_AUDIO_SINK@ 5%%-");
		BIND("XF86AudioMute", "exec wpctl set-mute @DEFAULT_AUDIO_SINK@ toggle");
		for (int i = 1; i <= t->workspaces && i <= 10; i++) {
			char k1[32], k2[32];

			snprintf(k1, sizeof k1, "$mod+%d", i == 10 ? 0 : i);
			snprintf(k2, sizeof k2, "$mod+Shift+%d", i == 10 ? 0 : i);
			BIND(k1, "workspace number %d", i);
			BIND(k2, "move container to workspace number %d", i);
		}
#undef BIND
	}
	fputs("\nmode \"resize\" {\n    bindsym Left resize shrink width 40px\n    bindsym Right resize grow width 40px\n", f);
	fputs("    bindsym Up resize shrink height 40px\n    bindsym Down resize grow height 40px\n", f);
	fputs("    bindsym Escape mode default\n    bindsym Return mode default\n}\n", f);

	/* esenciales de un sistema usable: contraseñas, red, Bluetooth, USB, portapapeles, bloqueo */
	fputs("\n# Servicios de la sesión\n", f);
	fputs("exec rros-polkit-agent\nexec nm-applet --indicator\nexec blueman-applet\nexec udiskie\n", f);
	fputs("exec wl-paste --watch cliphist store\nexec rros-first-login\n", f);
	if (t->lock_min || t->screen_off_min || t->suspend_min) {
		fputs("exec swayidle -w", f);
		if (t->lock_min)
			fprintf(f, " timeout %d 'swaylock -f'", t->lock_min * 60);
		if (t->screen_off_min)
			fprintf(f, " timeout %d 'swaymsg \"output * power off\"' resume 'swaymsg \"output * power on\"'", t->screen_off_min * 60);
		if (t->suspend_min)
			fprintf(f, " timeout %d 'systemctl suspend'", t->suspend_min * 60);
		fputs(" before-sleep 'swaylock -f'\n", f);
	}

	/* personalización del usuario: atajos, reglas por aplicación y programas de inicio */
	n = collect(c, "keybind.", items, 64);
	if (n)
		fputs("\n# Tus atajos (keybind.*)\n", f);
	for (size_t i = 0; i < n; i++) {
		if (rros_keybind_to_sway(items[i]->val, line, sizeof line) == 0)
			fprintf(f, "%s\n", line);
		else
			fprintf(f, "# ignorado (no válido): %s\n", items[i]->key);
	}
	n = collect(c, "rule.", items, 64);
	if (n)
		fputs("\n# Tus reglas de ventana (rule.*)\n", f);
	for (size_t i = 0; i < n; i++) {
		if (rros_rule_to_sway(items[i]->val, line, sizeof line) == 0)
			fprintf(f, "%s\n", line);
		else
			fprintf(f, "# ignorada (no válida): %s\n", items[i]->key);
	}
	/* el panel (waybar) lo lanza rros-panel desde exec: con `bar { swaybar_command }` sway lo arrancaba
	 * antes de que existiera el bus de la sesión y no se reintentaba */
	fputs("\nexec rros-panel\n\n", f);
	fputs("include ~/.config/sway/rros-effects.conf\n", f);
	fputs("exec mako\n", f);
	n = collect(c, "autostart.", items, 64);
	for (size_t i = 0; i < n; i++) {
		if (rros_autostart_to_sway(items[i]->val, line, sizeof line) == 0)
			fprintf(f, "%s\n", line);
		else
			fprintf(f, "# ignorado (no válido): %s\n", items[i]->key);
	}
}

void rros_render_effects(const struct rros_theme *t, const struct rros_palette *p,
			 const struct rros_conf *c, FILE *f)
{
	(void)p; (void)c;
	fprintf(f, "# %s (efectos de SwayFX).\n", RROS_GENERATED);
	if (!t->full_compositor || strcmp(t->effects, "off") == 0)
		return;
	fprintf(f, "corner_radius %d\n", t->radius);
	fprintf(f, "blur %s\n", t->blur ? "enable" : "disable");
	if (t->blur) {
		fprintf(f, "blur_radius %d\nblur_passes %d\n", strcmp(t->effects, "advanced") == 0 ? 7 : 4,
			strcmp(t->effects, "advanced") == 0 ? 3 : 2);
		fputs("layer_effects \"waybar\" blur enable\n", f);
	}
	fprintf(f, "shadows %s\n", t->shadows ? "enable" : "disable");
	if (t->shadows)
		fputs("shadow_blur_radius 20\nshadow_color #00000066\n", f);
	if (strcmp(t->effects, "advanced") == 0)
		fputs("default_dim_inactive 0.08\n", f);
}

/* ---- waybar ---- */

static const struct { const char *name, *module; } modmap[] = {
	{ "workspaces", "sway/workspaces" }, { "mode", "sway/mode" }, { "window", "sway/window" },
	{ "language", "sway/language" }, { "clock", "clock" }, { "profile", "custom/profile" },
	{ "memory", "memory" }, { "cpu", "cpu" }, { "network", "network" }, { "audio", "pulseaudio" },
	{ "battery", "battery" }, { "tray", "tray" }, { "disk", "disk" }, { "temperature", "temperature" },
	{ "backlight", "backlight" }, { "idle", "idle_inhibitor" },
	{ "apps", "custom/apps" }, { "power", "custom/power" },
};

static void emit_modules(FILE *f, const char *key, const char *list, int last)
{
	char buf[160], *tok, *save = NULL;
	int first = 1;

	snprintf(buf, sizeof buf, "%s", list);
	fprintf(f, "  \"%s\": [", key);
	for (tok = strtok_r(buf, ",", &save); tok; tok = strtok_r(NULL, ",", &save)) {
		for (size_t i = 0; i < sizeof modmap / sizeof *modmap; i++) {
			if (strcmp(tok, modmap[i].name))
				continue;
			fprintf(f, "%s\"%s\"", first ? "" : ", ", modmap[i].module);
			first = 0;
		}
	}
	fprintf(f, "]%s\n", last ? "" : ",");
}

void rros_render_waybar(const struct rros_theme *t, const struct rros_palette *p,
			const struct rros_conf *c, FILE *f)
{
	int vertical = !strcmp(t->panel_pos, "left") || !strcmp(t->panel_pos, "right");
	int floating = strcmp(t->panel_style, "flat") != 0;

	(void)p; (void)c;
	fprintf(f, "// %s. No lo edites.\n{\n", RROS_GENERATED);
	fprintf(f, "  \"layer\": \"top\",\n  \"position\": \"%s\",\n  \"%s\": %d,\n  \"spacing\": 4,\n",
		t->panel_pos, vertical ? "width" : "height", t->panel_size);
	if (floating) {
		const char *m = !strcmp(t->panel_pos, "top") ? "6 8 0 8" : !strcmp(t->panel_pos, "bottom") ? "0 8 6 8" :
				!strcmp(t->panel_pos, "left") ? "8 0 8 6" : "8 6 8 0";

		fprintf(f, "  \"margin\": \"%s\",\n", m);
	}
	emit_modules(f, "modules-left", t->panel_left, 0);
	emit_modules(f, "modules-center", t->panel_center, 0);
	emit_modules(f, "modules-right", t->panel_right, 0);
	fputs("  \"sway/workspaces\": { \"disable-scroll\": true },\n", f);
	fprintf(f, "  \"clock\": { \"format\": \"%s\", \"tooltip-format\": \"{:%%A %%d %%B %%Y}\" },\n",
		t->clock_24h ? "{:%H:%M}" : "{:%I:%M %p}");
	fputs("  \"custom/profile\": { \"exec\": \"rros-config profile\", \"interval\": \"once\", \"format\": \"RROS {}\" },\n", f);
	fputs("  \"memory\": { \"format\": \"RAM {used:0.1f}G\", \"interval\": 5 },\n", f);
	fputs("  \"cpu\": { \"format\": \"CPU {usage}%\", \"interval\": 5 },\n", f);
	fputs("  \"network\": { \"format-wifi\": \"{essid}\", \"format-ethernet\": \"LAN\", \"format-disconnected\": \"sin red\", \"on-click\": \"nm-connection-editor\" },\n", f);
	fputs("  \"pulseaudio\": { \"format\": \"VOL {volume}%\", \"format-muted\": \"MUTE\", \"on-click\": \"pavucontrol\", \"on-click-right\": \"wpctl set-mute @DEFAULT_AUDIO_SINK@ toggle\" },\n", f);
	fputs("  \"disk\": { \"format\": \"DISCO {percentage_used}%\", \"interval\": 60 },\n", f);
	fputs("  \"temperature\": { \"format\": \"{temperatureC}°C\" },\n", f);
	fputs("  \"backlight\": { \"format\": \"LUZ {percent}%\" },\n", f);
	fputs("  \"idle_inhibitor\": { \"format\": \"{icon}\", \"format-icons\": { \"activated\": \"CAFE\", \"deactivated\": \"cafe\" } },\n", f);
	fputs("  \"tray\": { \"spacing\": 8 },\n", f);
	fputs("  \"custom/apps\": { \"format\": \"Aplicaciones\", \"on-click\": \"fuzzel\", \"tooltip\": false },\n", f);
	fputs("  \"custom/power\": { \"format\": \"Apagar\", \"on-click\": \"rros-power\", \"tooltip\": false },\n", f);
	fprintf(f, "  \"battery\": { \"states\": { \"warning\": %d, \"critical\": 5 }, \"format\": \"%s\", \"interval\": 30 }\n}\n",
		t->battery_low, t->battery_percent ? "BAT {capacity}%" : "BAT");
}

void rros_render_waybar_css(const struct rros_theme *t, const struct rros_palette *p,
			    const struct rros_conf *c, FILE *f)
{
	int r, g, b, sr, sg, sb;
	double alpha = alpha_of(t, 0.15);
	const char *edge = !strcmp(t->panel_pos, "top") ? "bottom" : !strcmp(t->panel_pos, "bottom") ? "top" :
			   !strcmp(t->panel_pos, "left") ? "right" : "left";
	int flat = !strcmp(t->panel_style, "flat"), pill = !strcmp(t->panel_style, "pill");
	static const char *mods = "#clock, #memory, #cpu, #network, #pulseaudio, #battery, #disk, #temperature, "
				  "#backlight, #tray, #idle_inhibitor, #language, #mode, #window";

	(void)c;
	rgb_of(p->bg, &r, &g, &b);
	rgb_of(p->surface, &sr, &sg, &sb);
	fprintf(f, "/* %s. No lo edites. */\n", RROS_GENERATED);
	fprintf(f, "* { font-family: \"%s\", sans-serif; font-size: %dpx; border: none; border-radius: 0; min-height: 0; }\n",
		t->font, t->font_size + 2);
	if (flat) {
		fprintf(f, "window#waybar { background: rgba(%d, %d, %d, %.2f); color: %s; border-%s: 2px solid %s; }\n",
			r, g, b, alpha, p->text, edge, p->accent);
	} else {
		fprintf(f, "window#waybar { background: transparent; color: %s; }\n", p->text);
		if (pill)
			fprintf(f, "%s { background: rgba(%d, %d, %d, %.2f); border-radius: 999px; margin: 4px 2px; }\n",
				"#workspaces, #clock, #custom-profile, #memory, #cpu, #network, #pulseaudio, #battery, #disk, #temperature, #backlight, #tray, #idle_inhibitor, #language, #mode, #window",
				sr, sg, sb, alpha);
		else
			fprintf(f, ".modules-left, .modules-center, .modules-right { background: rgba(%d, %d, %d, %.2f); border: 1px solid %s; border-radius: %dpx; padding: 0 4px; }\n",
				r, g, b, alpha, p->border, t->radius);
	}
	fprintf(f, "#workspaces button { padding: 0 10px; margin: 3px 2px; color: %s; background: transparent; border-radius: %dpx; }\n",
		p->muted, pill ? 999 : t->radius);
	fprintf(f, "#workspaces button.focused { color: %s; background: %s; }\n", p->bg, p->accent);
	fprintf(f, "#workspaces button:hover { background: %s; color: %s; }\n", p->accent_dim, p->text);
	fprintf(f, "#custom-profile { color: %s; background: %s; border-radius: %dpx; padding: 0 12px; margin: 3px 4px; font-weight: bold; }\n",
		p->bg, p->accent, pill ? 999 : t->radius);
	fprintf(f, "%s { padding: 0 10px; color: %s; }\n", mods, p->text);
	fprintf(f, "#custom-apps, #custom-power { padding: 0 12px; margin: 3px 4px; border-radius: %dpx; background: %s; color: %s; }\n",
		pill ? 999 : t->radius, p->surface, p->text);
	fprintf(f, "#custom-apps:hover, #custom-power:hover { background: %s; color: %s; }\n", p->accent, p->bg);
	fprintf(f, "#clock { font-weight: bold; }\n#memory { color: %s; }\n#cpu { color: %s; }\n", p->cyan, p->magenta);
	fprintf(f, "#battery.warning { color: %s; }\n#battery.critical { color: %s; }\n", p->yellow, p->red);
	if (t->animations)
		fputs("#workspaces button { transition: background 150ms ease, color 150ms ease; }\n", f);
}

/* ---- foot ---- */

void rros_render_foot(const struct rros_theme *t, const struct rros_palette *p,
		      const struct rros_conf *c, FILE *f)
{
	(void)c;
	fprintf(f, "# %s. No lo edites.\n", RROS_GENERATED);
	fprintf(f, "[main]\nfont=%s:size=%d\npad=%dx%d\n\n", t->font_mono, t->font_size, t->term_padding,
		t->term_padding * 4 / 5);
	fprintf(f, "[scrollback]\nlines=%d\n\n", t->term_scrollback);
	fprintf(f, "[colors]\nalpha=%.2f\nbackground=%s\nforeground=%s\n", alpha_of(t, 0.3), nh(p->bg), nh(p->text));
	fprintf(f, "regular0=%s\nregular1=%s\nregular2=%s\nregular3=%s\n", nh(p->surface), nh(p->red), nh(p->green), nh(p->yellow));
	fprintf(f, "regular4=%s\nregular5=%s\nregular6=%s\nregular7=%s\n", nh(p->blue), nh(p->magenta), nh(p->cyan), nh(p->text));
	fprintf(f, "bright0=%s\nbright1=%s\nbright2=%s\nbright3=%s\n", nh(p->muted), nh(p->red), nh(p->green), nh(p->yellow));
	fprintf(f, "bright4=%s\nbright5=%s\nbright6=%s\nbright7=%s\n", nh(p->blue), nh(p->magenta), nh(p->cyan), nh(p->text));
	fprintf(f, "selection-foreground=%s\nselection-background=%s\n\n", nh(p->bg), nh(p->accent));
	fprintf(f, "[cursor]\nstyle=%s\ncolor=%s %s\n", t->term_cursor, nh(p->bg), nh(p->accent));
}

/* ---- notificaciones, lanzador y GTK ---- */

void rros_render_mako(const struct rros_theme *t, const struct rros_palette *p,
		      const struct rros_conf *c, FILE *f)
{
	char bg[16];

	(void)c;
	hex_alpha(bg, sizeof bg, p->bg, alpha_of(t, 0.5), 1);
	fprintf(f, "# %s. No lo edites.\n", RROS_GENERATED);
	fprintf(f, "font=%s %d\nbackground-color=%s\ntext-color=%s\nborder-color=%s\n", t->font, t->font_size, bg, p->text, p->accent);
	fprintf(f, "border-size=%d\nborder-radius=%d\nprogress-color=over %s\n", t->border, t->radius, p->surface);
	fputs("anchor=top-right\nmargin=12\npadding=12\ndefault-timeout=6000\nicons=1\n", f);
}

void rros_render_fuzzel(const struct rros_theme *t, const struct rros_palette *p,
			const struct rros_conf *c, FILE *f)
{
	char bg[16], acc[16], sur[16], txt[16];

	(void)c;
	hex_alpha(bg, sizeof bg, p->bg, alpha_of(t, 0.5), 0);
	hex_alpha(acc, sizeof acc, p->accent, 1.0, 0);
	hex_alpha(sur, sizeof sur, p->surface, 1.0, 0);
	hex_alpha(txt, sizeof txt, p->text, 1.0, 0);
	fprintf(f, "# %s. No lo edites.\n[main]\nfont=%s:size=%d\nterminal=foot\nprompt=\"> \"\n\n", RROS_GENERATED, t->font, t->font_size + 1);
	fprintf(f, "[colors]\nbackground=%s\ntext=%s\nmatch=%s\nselection=%s\nselection-text=%s\nselection-match=%s\nborder=%s\n\n",
		bg, txt, acc, sur, txt, acc, acc);
	fprintf(f, "[border]\nwidth=%d\nradius=%d\n", t->border, t->radius);
}

void rros_render_swaylock(const struct rros_theme *t, const struct rros_palette *p,
			  const struct rros_conf *c, FILE *f)
{
	(void)c;
	fprintf(f, "# %s. No lo edites.\n", RROS_GENERATED);
	fprintf(f, "color=%s\ninside-color=%s\nring-color=%s\nkey-hl-color=%s\nbs-hl-color=%s\n", nh(p->bg), nh(p->surface),
		nh(p->border), nh(p->accent), nh(p->red));
	fprintf(f, "line-color=%s\nseparator-color=%s\ntext-color=%s\n", nh(p->bg), nh(p->bg), nh(p->text));
	fprintf(f, "inside-ver-color=%s\nring-ver-color=%s\ninside-wrong-color=%s\nring-wrong-color=%s\n", nh(p->surface),
		nh(p->green), nh(p->surface), nh(p->red));
	fprintf(f, "font=%s\nindicator-radius=80\nindicator-thickness=8\nshow-failed-attempts\nignore-empty-password\n", t->font);
}

void rros_render_gtk_settings(const struct rros_theme *t, const struct rros_palette *p,
			      const struct rros_conf *c, FILE *f)
{
	(void)p; (void)c;
	fprintf(f, "# %s. No lo edites.\n[Settings]\n", RROS_GENERATED);
	fprintf(f, "gtk-application-prefer-dark-theme=%d\ngtk-icon-theme-name=%s\ngtk-font-name=%s %d\n",
		t->dark, t->icon_theme, t->font, t->font_size);
	fprintf(f, "gtk-cursor-theme-name=%s\ngtk-cursor-theme-size=%d\n", t->cursor_theme, t->cursor_size);
}

void rros_render_gtk_css(const struct rros_theme *t, const struct rros_palette *p,
			 const struct rros_conf *c, FILE *f)
{
	(void)t; (void)c;
	fprintf(f, "/* %s. No lo edites. */\n", RROS_GENERATED);
	/* nombres de libadwaita (GTK4) */
	fprintf(f, "@define-color accent_bg_color %s;\n@define-color accent_color %s;\n@define-color accent_fg_color %s;\n", p->accent, p->accent, p->bg);
	fprintf(f, "@define-color window_bg_color %s;\n@define-color window_fg_color %s;\n", p->bg, p->text);
	fprintf(f, "@define-color view_bg_color %s;\n@define-color view_fg_color %s;\n", p->surface, p->text);
	fprintf(f, "@define-color headerbar_bg_color %s;\n@define-color headerbar_fg_color %s;\n", p->surface, p->text);
	fprintf(f, "@define-color card_bg_color %s;\n@define-color card_fg_color %s;\n", p->surface, p->text);
	fprintf(f, "@define-color popover_bg_color %s;\n@define-color popover_fg_color %s;\n", p->surface, p->text);
	fprintf(f, "@define-color destructive_bg_color %s;\n@define-color error_color %s;\n", p->red, p->red);
	fprintf(f, "@define-color success_color %s;\n@define-color warning_color %s;\n", p->green, p->yellow);
	/* nombres de GTK3 */
	fprintf(f, "@define-color theme_bg_color %s;\n@define-color theme_fg_color %s;\n", p->bg, p->text);
	fprintf(f, "@define-color theme_base_color %s;\n@define-color theme_text_color %s;\n", p->surface, p->text);
	fprintf(f, "@define-color theme_selected_bg_color %s;\n@define-color theme_selected_fg_color %s;\n", p->accent, p->bg);
}
