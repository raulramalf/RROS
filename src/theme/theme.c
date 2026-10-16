#include "theme.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "rros_schema.h"

#define DEFAULT_ACCENT "#7c5cff"

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

static void mix(const char *a, const char *b, int pct_b, char out[8])
{
	int ar, ag, ab, br, bg, bb;

	rgb_of(a, &ar, &ag, &ab);
	rgb_of(b, &br, &bg, &bb);
	snprintf(out, 8, "#%02x%02x%02x", (ar * (100 - pct_b) + br * pct_b) / 100,
		 (ag * (100 - pct_b) + bg * pct_b) / 100, (ab * (100 - pct_b) + bb * pct_b) / 100);
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

static void cfg_enum(const struct rros_conf *c, const char *key, const char *def,
		     char *dst, size_t n)
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
	char scheme[8];

	memset(t, 0, sizeof *t);
	snprintf(t->accent, sizeof t->accent, "%s", (acc && valid_hex(acc)) ? acc : DEFAULT_ACCENT);
	for (char *p = t->accent; *p; p++)
		*p = (char)tolower((unsigned char)*p);
	cfg_enum(c, "ui.color_scheme", "dark", scheme, sizeof scheme);
	t->dark = strcmp(scheme, "light") != 0;
	cfg_str(c, "ui.font", "Inter", " ._-", t->font, sizeof t->font);
	cfg_str(c, "ui.font_mono", "JetBrains Mono", " ._-", t->font_mono, sizeof t->font_mono);
	t->font_size = (int)cfg_int(c, "ui.font_size", 11);
	t->scale = (int)cfg_int(c, "ui.scale", 100);
	t->gap = (int)cfg_int(c, "ui.gap", 8);
	t->border = (int)cfg_int(c, "ui.border_width", 2);
	t->radius = (int)cfg_int(c, "ui.corner_radius", 8);
	t->panel_size = (int)cfg_int(c, "ui.panel_size", 32);
	t->workspaces = (int)cfg_int(c, "ui.workspaces", 4);
	t->transparency = (int)cfg_int(c, "ui.transparency", 0);
	cfg_enum(c, "ui.panel_position", "top", t->panel_pos, sizeof t->panel_pos);
	cfg_enum(c, "ui.layout", "hybrid", t->layout, sizeof t->layout);
	cfg_enum(c, "ui.effects", "basic", t->effects, sizeof t->effects);
	cfg_str(c, "input.keymap", "es", "_,", t->keymap, sizeof t->keymap);
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
	{
		char comp[8];

		cfg_enum(c, "ui.compositor", "minimal", comp, sizeof comp);
		t->full_compositor = strcmp(comp, "full") == 0;
	}
}

static void set(char *dst, const char *v)
{
	snprintf(dst, 8, "%s", v);
}

void rros_theme_palette(const struct rros_theme *t, struct rros_palette *p)
{
	if (t->dark) {
		set(p->bg, "#0b1020"); set(p->surface, "#141c38"); set(p->text, "#e6ecff");
		set(p->muted, "#8a96bd"); set(p->border, "#26305a");
		set(p->red, "#ff6b81"); set(p->green, "#4ade80"); set(p->yellow, "#fbbf24");
		set(p->blue, "#60a5fa"); set(p->magenta, "#c084fc"); set(p->cyan, "#22d3ee");
	} else {
		set(p->bg, "#f5f7ff"); set(p->surface, "#ffffff"); set(p->text, "#141a33");
		set(p->muted, "#5b6489"); set(p->border, "#d5dbf5");
		set(p->red, "#d6334f"); set(p->green, "#16a34a"); set(p->yellow, "#b7791f");
		set(p->blue, "#2563eb"); set(p->magenta, "#9333ea"); set(p->cyan, "#0891b2");
	}
	set(p->accent, t->accent);
	mix(t->accent, p->bg, 55, p->accent_dim);
}

/* ---- sway ---- */

void rros_render_sway(const struct rros_theme *t, const struct rros_palette *p, FILE *f)
{
	fprintf(f, "# %s. No lo edites: usa `rros-config set ui.<clave> <valor>`.\n", RROS_GENERATED);
	fputs("set $mod Mod4\nset $term foot\nset $menu fuzzel\n\n", f);
	fprintf(f, "font pango:%s %d\n", t->font, t->font_size);
	fprintf(f, "default_border pixel %d\ndefault_floating_border pixel %d\n", t->border, t->border);
	fprintf(f, "gaps inner %d\ngaps outer %d\nsmart_borders on\n", t->gap, t->gap / 2);
	fputs("focus_follows_mouse yes\nfloating_modifier $mod normal\n\n", f);
	fprintf(f, "client.focused          %s %s %s %s %s\n", p->accent, p->accent, p->text, p->accent, p->accent);
	fprintf(f, "client.focused_inactive %s %s %s %s %s\n", p->border, p->surface, p->muted, p->border, p->border);
	fprintf(f, "client.unfocused        %s %s %s %s %s\n", p->border, p->surface, p->muted, p->border, p->border);
	fprintf(f, "client.urgent           %s %s %s %s %s\n", p->red, p->red, p->bg, p->red, p->red);
	fprintf(f, "client.background       %s\n", p->bg);
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

	fputs("\n# Atajos RROS\n", f);
	fputs("bindsym $mod+Return exec $term\nbindsym $mod+space exec $menu\nbindsym $mod+d exec $menu\n", f);
	fputs("bindsym $mod+q kill\nbindsym $mod+f fullscreen\nbindsym $mod+Shift+space floating toggle\n", f);
	fputs("bindsym $mod+Shift+r reload\nbindsym $mod+Shift+e exec swaymsg exit\n", f);
	fputs("bindsym $mod+t exec rros-theme toggle\n", f);
	fputs("bindsym $mod+p exec foot -T RROS -e sh -c 'rros-fetch; rros doctor; read -r _'\n", f);
	fputs("bindsym $mod+Left focus left\nbindsym $mod+Right focus right\n", f);
	fputs("bindsym $mod+Up focus up\nbindsym $mod+Down focus down\n", f);
	fputs("bindsym $mod+h focus left\nbindsym $mod+l focus right\nbindsym $mod+k focus up\nbindsym $mod+j focus down\n", f);
	fputs("bindsym $mod+s layout stacking\nbindsym $mod+w layout tabbed\nbindsym $mod+e layout toggle split\n", f);
	fputs("bindsym Print exec rros-shot\nbindsym $mod+Shift+s exec rros-shot region\n", f);
	fputs("bindsym XF86MonBrightnessUp exec brightnessctl set +5%\n", f);
	fputs("bindsym XF86MonBrightnessDown exec brightnessctl set 5%-\n", f);
	fputs("bindsym XF86AudioRaiseVolume exec wpctl set-volume -l 1.5 @DEFAULT_AUDIO_SINK@ 5%+\n", f);
	fputs("bindsym XF86AudioLowerVolume exec wpctl set-volume @DEFAULT_AUDIO_SINK@ 5%-\n", f);
	fputs("bindsym XF86AudioMute exec wpctl set-mute @DEFAULT_AUDIO_SINK@ toggle\n", f);
	for (int i = 1; i <= t->workspaces && i <= 10; i++) {
		int key = i == 10 ? 0 : i;

		fprintf(f, "bindsym $mod+%d workspace number %d\n", key, i);
		fprintf(f, "bindsym $mod+Shift+%d move container to workspace number %d\n", key, i);
	}
	fputs("\nmode \"resize\" {\n    bindsym Left resize shrink width 40px\n    bindsym Right resize grow width 40px\n", f);
	fputs("    bindsym Up resize shrink height 40px\n    bindsym Down resize grow height 40px\n", f);
	fputs("    bindsym Escape mode default\n    bindsym Return mode default\n}\nbindsym $mod+r mode \"resize\"\n", f);
	fputs("\nbar {\n    swaybar_command waybar\n}\n\n", f);
	fputs("include ~/.config/sway/rros-effects.conf\n", f);
	fputs("exec mako\n", f);
}

void rros_render_effects(const struct rros_theme *t, const struct rros_palette *p, FILE *f)
{
	(void)p;
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

void rros_render_waybar(const struct rros_theme *t, const struct rros_palette *p, FILE *f)
{
	int vertical = !strcmp(t->panel_pos, "left") || !strcmp(t->panel_pos, "right");

	(void)p;
	fprintf(f, "// %s. No lo edites.\n{\n", RROS_GENERATED);
	fprintf(f, "  \"layer\": \"top\",\n  \"position\": \"%s\",\n  \"%s\": %d,\n  \"spacing\": 4,\n",
		t->panel_pos, vertical ? "width" : "height", t->panel_size);
	fputs("  \"modules-left\": [\"sway/workspaces\", \"sway/mode\"],\n  \"modules-center\": [\"clock\"],\n", f);
	fputs("  \"modules-right\": [\"custom/profile\", \"memory\", \"cpu\", \"network\", \"pulseaudio\", \"battery\"],\n", f);
	fputs("  \"sway/workspaces\": { \"disable-scroll\": true },\n", f);
	fprintf(f, "  \"clock\": { \"format\": \"%s\", \"tooltip-format\": \"{:%%A %%d %%B %%Y}\" },\n",
		t->clock_24h ? "{:%H:%M}" : "{:%I:%M %p}");
	fputs("  \"custom/profile\": { \"exec\": \"rros-config profile\", \"interval\": \"once\", \"format\": \"RROS {}\" },\n", f);
	fputs("  \"memory\": { \"format\": \"RAM {used:0.1f}G\", \"interval\": 5 },\n", f);
	fputs("  \"cpu\": { \"format\": \"CPU {usage}%\", \"interval\": 5 },\n", f);
	fputs("  \"network\": { \"format-wifi\": \"{essid}\", \"format-ethernet\": \"LAN\", \"format-disconnected\": \"sin red\" },\n", f);
	fputs("  \"pulseaudio\": { \"format\": \"VOL {volume}%\", \"format-muted\": \"MUTE\", \"on-click\": \"wpctl set-mute @DEFAULT_AUDIO_SINK@ toggle\" },\n", f);
	fprintf(f, "  \"battery\": { \"states\": { \"warning\": %d, \"critical\": 5 }, \"format\": \"%s\", \"interval\": 30 }\n}\n",
		t->battery_low, t->battery_percent ? "BAT {capacity}%" : "BAT");
}

void rros_render_waybar_css(const struct rros_theme *t, const struct rros_palette *p, FILE *f)
{
	int r, g, b;
	double alpha = 1.0 - t->transparency / 100.0;
	const char *edge = !strcmp(t->panel_pos, "top") ? "bottom" : !strcmp(t->panel_pos, "bottom") ? "top" :
			   !strcmp(t->panel_pos, "left") ? "right" : "left";

	if (alpha < 0.15)
		alpha = 0.15;
	rgb_of(p->bg, &r, &g, &b);
	fprintf(f, "/* %s. No lo edites. */\n", RROS_GENERATED);
	fprintf(f, "* { font-family: \"%s\", sans-serif; font-size: %dpx; border: none; border-radius: 0; min-height: 0; }\n",
		t->font, t->font_size + 2);
	fprintf(f, "window#waybar { background: rgba(%d, %d, %d, %.2f); color: %s; border-%s: 2px solid %s; }\n",
		r, g, b, alpha, p->text, edge, p->accent);
	fprintf(f, "#workspaces button { padding: 0 10px; margin: 3px 2px; color: %s; background: transparent; border-radius: %dpx; }\n",
		p->muted, t->radius);
	fprintf(f, "#workspaces button.focused { color: %s; background: %s; }\n", p->bg, p->accent);
	fprintf(f, "#workspaces button:hover { background: %s; color: %s; }\n", p->accent_dim, p->text);
	fprintf(f, "#custom-profile { color: %s; background: %s; border-radius: %dpx; padding: 0 12px; margin: 3px 4px; font-weight: bold; }\n",
		p->bg, p->accent, t->radius);
	fprintf(f, "#clock, #memory, #cpu, #network, #pulseaudio, #battery { padding: 0 10px; color: %s; }\n", p->text);
	fprintf(f, "#clock { font-weight: bold; }\n#memory { color: %s; }\n#cpu { color: %s; }\n", p->cyan, p->magenta);
	fprintf(f, "#battery.warning { color: %s; }\n#battery.critical { color: %s; }\n", p->yellow, p->red);
	if (t->animations)
		fputs("#workspaces button { transition: background 150ms ease, color 150ms ease; }\n", f);
}

/* ---- foot ---- */

static const char *nh(const char *hex)
{
	return hex + 1;
}

void rros_render_foot(const struct rros_theme *t, const struct rros_palette *p, FILE *f)
{
	double alpha = 1.0 - t->transparency / 100.0;

	if (alpha < 0.3)
		alpha = 0.3;
	fprintf(f, "# %s. No lo edites.\n", RROS_GENERATED);
	fprintf(f, "[main]\nfont=%s:size=%d\npad=10x8\n\n", t->font_mono, t->font_size);
	fprintf(f, "[colors]\nalpha=%.2f\nbackground=%s\nforeground=%s\n", alpha, nh(p->bg), nh(p->text));
	fprintf(f, "regular0=%s\nregular1=%s\nregular2=%s\nregular3=%s\n", nh(p->surface), nh(p->red), nh(p->green), nh(p->yellow));
	fprintf(f, "regular4=%s\nregular5=%s\nregular6=%s\nregular7=%s\n", nh(p->blue), nh(p->magenta), nh(p->cyan), nh(p->text));
	fprintf(f, "bright0=%s\nbright1=%s\nbright2=%s\nbright3=%s\n", nh(p->muted), nh(p->red), nh(p->green), nh(p->yellow));
	fprintf(f, "bright4=%s\nbright5=%s\nbright6=%s\nbright7=%s\n", nh(p->blue), nh(p->magenta), nh(p->cyan), nh(p->text));
	fprintf(f, "selection-foreground=%s\nselection-background=%s\n\n", nh(p->bg), nh(p->accent));
	fprintf(f, "[cursor]\ncolor=%s %s\n", nh(p->bg), nh(p->accent));
}
