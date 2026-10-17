/* Hyprland: ~/.config/hypr/hyprland.conf y hyprlock.conf desde las mismas claves que sway.
 * Las opciones que cambian de nombre entre versiones de Hyprland van entre
 * `# hyprlang noerror true/false`: la versión instalada usa la que entiende y calla las demás. */
#include "theme.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "rros_schema.h"

static int by_key(const void *a, const void *b)
{
	return strcmp((*(const struct rros_kv *const *)a)->key, (*(const struct rros_kv *const *)b)->key);
}

static size_t collect(const struct rros_conf *c, const char *prefix, const struct rros_kv **out, size_t max)
{
	size_t n = 0, pl = strlen(prefix);

	for (size_t i = 0; i < c->n && n < max; i++)
		if (!strncmp(c->items[i].key, prefix, pl))
			out[n++] = &c->items[i];
	qsort(out, n, sizeof *out, by_key);
	return n;
}

/* "#rrggbb" + alfa -> "rgba(rrggbbaa)" */
static const char *rgba(char *buf, size_t n, const char *hex, double alpha)
{
	int a = (int)(alpha * 255 + 0.5);

	snprintf(buf, n, "rgba(%s%02x)", hex + 1, a < 0 ? 0 : a > 255 ? 255 : a);
	return buf;
}

/* velocidad de una animación (décimas de segundo) escalada con ui.animation_speed */
static int speed(const struct rros_theme *t, int base)
{
	int v = (base * 100 + t->animation_speed / 2) / (t->animation_speed ? t->animation_speed : 100);

	return v < 1 ? 1 : v > 50 ? 50 : v;
}

/* combinaciones de los atajos del usuario, para que sustituyan a las de RROS ("SUPER SHIFT, b") */
struct combos {
	char v[64][64];
	size_t n;
};

static void user_combos(const struct rros_conf *c, struct combos *u)
{
	const struct rros_kv *kb[64];
	size_t n = collect(c, "keybind.", kb, 64);
	char tmp[600];

	u->n = 0;
	for (size_t i = 0; i < n && u->n < 64; i++) {
		char *second;

		if (rros_keybind_to_hypr(kb[i]->val, tmp, sizeof tmp))
			continue;
		second = strchr(tmp, ',');
		second = second ? strchr(second + 1, ',') : NULL;
		if (second && (size_t)(second - tmp) < sizeof u->v[0]) {
			memcpy(u->v[u->n], tmp, (size_t)(second - tmp));
			u->v[u->n][second - tmp] = '\0';
			u->n++;
		}
	}
}

static void bind(FILE *f, const struct combos *u, const char *kind, const char *mods, const char *key, const char *action)
{
	char combo[80];

	snprintf(combo, sizeof combo, "%s, %s", mods, key);
	for (size_t i = 0; i < u->n; i++)
		if (!strcasecmp(u->v[i], combo))
			return;
	fprintf(f, "%s = %s, %s\n", kind, combo, action);
}

void rros_render_hyprland(const struct rros_theme *t, const struct rros_palette *p,
			  const struct rros_conf *c, FILE *f)
{
	const struct rros_kv *items[64];
	char a[24], b[24], wp[300], line[600];
	struct combos u;
	size_t n;
	int effects = strcmp(t->effects, "off") != 0, floating = !strcmp(t->layout, "floating");
	int advanced = !strcmp(t->effects, "advanced");
	double inactive = 1.0 - t->transparency / 200.0;

	fprintf(f, "# %s. No lo edites: usa RROS Ajustes o `rros-config set ui.<clave> <valor>`.\n", RROS_GENERATED);
	fprintf(f, "# Tus propias líneas van en ~/.config/hypr/personal.conf (se incluye al final).\n\n");
	fprintf(f, "$term = %s\n$menu = fuzzel\n$browser = %s\n$files = %s\n$editor = %s\n\n", t->app_term,
		t->app_browser, t->app_files, t->app_editor);
	fprintf(f, "monitor = , preferred, auto, %.2f\n\n", t->scale / 100.0);
	fprintf(f, "env = XCURSOR_THEME,%s\nenv = XCURSOR_SIZE,%d\n", t->cursor_theme, t->cursor_size);
	fputs("env = QT_QPA_PLATFORM,wayland;xcb\nenv = MOZ_ENABLE_WAYLAND,1\nenv = _JAVA_AWT_WM_NONREPARENTING,1\n\n", f);

	/* servicios de la sesión: los mismos que con sway */
	fputs("# Servicios de la sesión\n", f);
	fputs("exec-once = dbus-update-activation-environment --systemd WAYLAND_DISPLAY DISPLAY HYPRLAND_INSTANCE_SIGNATURE "
	      "XDG_CURRENT_DESKTOP XDG_SESSION_TYPE XDG_SESSION_DESKTOP\n", f);
	rros_wallpaper_path(t, p, wp, sizeof wp);
	if (wp[0])
		fprintf(f, "exec-once = swaybg -i %s -m %s\n", wp, t->wallpaper[0] ? t->wallpaper_mode : "fill");
	else
		fprintf(f, "exec-once = swaybg -c '%s'\n", p->bg);
	fputs("exec-once = rros-panel\nexec-once = mako\nexec-once = rros-polkit-agent\n", f);
	fputs("exec-once = nm-applet --indicator\nexec-once = blueman-applet\nexec-once = udiskie\n", f);
	fputs("exec-once = wl-paste --watch cliphist store\nexec-once = rros-first-login\n", f);
	if (t->dock)
		fputs("exec-once = rros-dock\n", f);
	if (t->screen_reader)
		fputs("exec-once = rros-a11y lector on\n", f);
	if (t->osk)
		fputs("exec-once = rros-a11y teclado on\n", f);
	if (t->lock_min || t->screen_off_min || t->suspend_min) {
		fputs("exec-once = swayidle -w", f);
		if (t->lock_min)
			fprintf(f, " timeout %d 'rros-lock -f'", t->lock_min * 60);
		if (t->screen_off_min)
			fprintf(f, " timeout %d 'hyprctl dispatch dpms off' resume 'hyprctl dispatch dpms on'", t->screen_off_min * 60);
		if (t->suspend_min)
			fprintf(f, " timeout %d 'systemctl suspend'", t->suspend_min * 60);
		fputs(" before-sleep 'rros-lock -f'\n", f);
	}

	fputs("\ninput {\n", f);
	fprintf(f, "    kb_layout = %s\n    repeat_delay = %d\n    repeat_rate = %d\n", t->keymap, t->repeat_delay, t->repeat_rate);
	fprintf(f, "    numlock_by_default = %s\n", t->numlock ? "true" : "false");
	/* ventanas libres: clic para elegir (como en Windows); mosaico: el foco sigue al ratón */
	fprintf(f, "    follow_mouse = %d\n    sensitivity = %.2f\n", floating ? 2 : 1, t->pointer_speed / 100.0);
	fprintf(f, "    touchpad {\n        natural_scroll = %s\n        tap-to-click = %s\n        disable_while_typing = %s\n    }\n}\n",
		t->natural_scroll ? "true" : "false", t->tap_to_click ? "true" : "false", t->dwt ? "true" : "false");

	fputs("\ngeneral {\n", f);
	fprintf(f, "    gaps_in = %d\n    gaps_out = %d\n    border_size = %d\n", (t->gap + 1) / 2, t->gap, t->border);
	if (t->border_gradient && !t->high_contrast)
		fprintf(f, "    col.active_border = %s %s 45deg\n", rgba(a, sizeof a, p->accent, 1), rgba(b, sizeof b, p->magenta, 1));
	else
		fprintf(f, "    col.active_border = %s\n", rgba(a, sizeof a, p->accent, 1));
	fprintf(f, "    col.inactive_border = %s\n", rgba(a, sizeof a, p->border, 0.8));
	fprintf(f, "    layout = %s\n    resize_on_border = true\n}\n", t->tiling);

	fputs("\ndecoration {\n", f);
	fprintf(f, "    rounding = %d\n    active_opacity = 1.0\n    inactive_opacity = %.2f\n", t->radius, inactive < 0.5 ? 0.5 : inactive);
	fprintf(f, "    dim_inactive = %s\n    dim_strength = 0.12\n", t->dim_inactive || advanced ? "true" : "false");
	fprintf(f, "    blur {\n        enabled = %s\n        size = %d\n        passes = %d\n        vibrancy = 0.17\n    }\n",
		effects && t->blur ? "true" : "false", t->blur_strength, advanced ? 3 : 2);
	fprintf(f, "    shadow {\n        enabled = %s\n        range = %d\n        render_power = 3\n        color = rgba(00000066)\n    }\n}\n",
		effects && t->shadows ? "true" : "false", advanced ? 24 : 14);

	/* animaciones: estilo y velocidad (ui.animation_style, ui.animation_speed) */
	fputs("\nanimations {\n", f);
	fprintf(f, "    enabled = %s\n", t->animations ? "true" : "false");
	{
		const char *bz = "0.05, 0.9, 0.1, 1.05", *win = "popin 80%", *ws = "slide";
		int base = 6;

		if (!strcmp(t->animation_style, "rapida")) {
			bz = "0.2, 0, 0, 1";
			win = "popin 90%";
			ws = "fade";
			base = 3;
		} else if (!strcmp(t->animation_style, "rebote")) {
			bz = "0.68, -0.55, 0.265, 1.55";
			base = 7;
		} else if (!strcmp(t->animation_style, "deslizar")) {
			bz = "0.25, 1, 0.5, 1";
			win = "slide";
			ws = "slidevert";
		}
		fprintf(f, "    bezier = rros, %s\n", bz);
		fprintf(f, "    animation = windows, 1, %d, rros, %s\n", speed(t, base), win);
		fprintf(f, "    animation = windowsOut, 1, %d, rros, %s\n", speed(t, base - 1 > 0 ? base - 1 : 1), win);
		fprintf(f, "    animation = border, 1, %d, default\n", speed(t, 10));
		fprintf(f, "    animation = fade, 1, %d, default\n", speed(t, base));
		fprintf(f, "    animation = workspaces, 1, %d, rros, %s\n}\n", speed(t, base), ws);
	}

	fputs("\ndwindle {\n    pseudotile = true\n    preserve_split = true\n}\n", f);
	fputs("master {\n    new_status = master\n}\n", f);
	fputs("misc {\n    force_default_wallpaper = 0\n    disable_hyprland_logo = true\n    disable_splash_rendering = true\n", f);
	fprintf(f, "    mouse_move_enables_dpms = true\n    key_press_enables_dpms = true\n    vrr = %d\n}\n",
		!strcmp(t->vrr, "on") ? 1 : !strcmp(t->vrr, "auto") ? 2 : 0);

	/* lo que cambia de sintaxis entre versiones de Hyprland */
	fputs("\n# hyprlang noerror true\n", f);
	if (t->gestures) {
		fputs("gesture = 3, horizontal, workspace\n", f);                              /* 0.51 y posteriores */
		fputs("gestures {\n    workspace_swipe = true\n    workspace_swipe_fingers = 3\n}\n", f);  /* anteriores */
	}
	if (floating) {
		fputs("windowrule = float, class:.*\n", f);
		fputs("windowrulev2 = float, class:.*\n", f);
	} else if (!strcmp(t->layout, "hybrid")) {
		/* pestañas: cada ventana nueva entra en el grupo de la que tiene el foco */
		fputs("windowrule = group set, class:.*\n", f);
		fputs("windowrulev2 = group set, class:.*\n", f);
	}
	n = collect(c, "rule.", items, 64);
	for (size_t i = 0; i < n; i++) {
		if (rros_rule_to_hypr(items[i]->val, line, sizeof line) == 0)
			fprintf(f, "windowrule = %s\nwindowrulev2 = %s\n", line, line);
		else
			fprintf(f, "# ignorada (no válida en Hyprland): %s\n", items[i]->key);
	}
	fputs("# hyprlang noerror false\n", f);

	if (strcmp(t->layout, "hybrid") == 0) {
		fprintf(f, "\ngroup {\n    col.border_active = %s\n", rgba(a, sizeof a, p->accent, 1));
		fprintf(f, "    col.border_inactive = %s\n", rgba(a, sizeof a, p->border, 0.8));
		fprintf(f, "    groupbar {\n        font_size = %d\n        col.active = %s\n", t->font_size + 1, rgba(a, sizeof a, p->accent, 1));
		fprintf(f, "        col.inactive = %s\n    }\n}\n", rgba(a, sizeof a, p->surface, 1));
	}

	/* atajos: los de RROS, salvo los que el usuario redefine */
	user_combos(c, &u);
	fputs("\n# Atajos RROS (Super = tecla de Windows)\n", f);
	bind(f, &u, "bind", "SUPER", "Return", "exec, $term");
	bind(f, &u, "bind", "SUPER", "space", "exec, $menu");
	bind(f, &u, "bind", "SUPER", "D", "exec, $menu");
	bind(f, &u, "bind", "SUPER", "Q", "killactive,");
	bind(f, &u, "bind", "SUPER", "F", "fullscreen, 0");
	bind(f, &u, "bind", "SUPER", "M", "fullscreen, 1");
	bind(f, &u, "bind", "SUPER SHIFT", "space", "togglefloating,");
	bind(f, &u, "bind", "SUPER SHIFT", "R", "exec, hyprctl reload");
	bind(f, &u, "bind", "SUPER SHIFT", "E", "exec, rros-power");
	bind(f, &u, "bind", "SUPER", "Escape", "exec, rros-lock");
	bind(f, &u, "bind", "SUPER", "L", "exec, rros-lock");
	bind(f, &u, "bind", "SUPER", "B", "exec, $browser");
	bind(f, &u, "bind", "SUPER", "E", "exec, $files");
	bind(f, &u, "bind", "SUPER SHIFT", "N", "exec, $editor");
	bind(f, &u, "bind", "SUPER", "V", "exec, rros-clipboard");
	bind(f, &u, "bind", "SUPER SHIFT", "W", "exec, nm-connection-editor");
	bind(f, &u, "bind", "SUPER", "T", "exec, rros-theme toggle");
	bind(f, &u, "bind", "SUPER SHIFT", "T", "exec, rros-theme cycle");
	bind(f, &u, "bind", "SUPER ALT", "W", "exec, rros-theme wallpaper --next");
	bind(f, &u, "bind", "SUPER", "comma", "exec, rros-settings open");
	bind(f, &u, "bind", "SUPER", "P", "exec, foot -T RROS -e sh -c 'rros-fetch; rros doctor; read -r _'");
	bind(f, &u, "bind", "SUPER ALT", "S", "exec, rros-a11y lector");
	bind(f, &u, "bind", "SUPER ALT", "K", "exec, rros-a11y teclado");
	bind(f, &u, "bind", "SUPER", "F1", "exec, rros-settings open bienvenida");
	bind(f, &u, "bind", "SUPER", "left", "movefocus, l");
	bind(f, &u, "bind", "SUPER", "right", "movefocus, r");
	bind(f, &u, "bind", "SUPER", "up", "movefocus, u");
	bind(f, &u, "bind", "SUPER", "down", "movefocus, d");
	bind(f, &u, "bind", "SUPER", "H", "movefocus, l");
	bind(f, &u, "bind", "SUPER", "K", "movefocus, u");
	bind(f, &u, "bind", "SUPER", "J", "movefocus, d");
	bind(f, &u, "bind", "SUPER SHIFT", "left", "movewindow, l");
	bind(f, &u, "bind", "SUPER SHIFT", "right", "movewindow, r");
	bind(f, &u, "bind", "SUPER SHIFT", "up", "movewindow, u");
	bind(f, &u, "bind", "SUPER SHIFT", "down", "movewindow, d");
	bind(f, &u, "bind", "SUPER", "W", "togglegroup,");
	bind(f, &u, "bind", "SUPER", "Tab", "changegroupactive, f");
	bind(f, &u, "bind", "ALT", "Tab", "cyclenext,");
	bind(f, &u, "bind", "SUPER", "G", "togglesplit,");
	bind(f, &u, "bind", "SUPER", "R", "submap, resize");
	bind(f, &u, "bind", "SUPER", "S", "togglespecialworkspace,");
	bind(f, &u, "bind", "SUPER SHIFT", "S", "exec, rros-shot region");
	bind(f, &u, "bind", "", "Print", "exec, rros-shot");
	bind(f, &u, "bindel", "", "XF86MonBrightnessUp", "exec, brightnessctl set +5%");
	bind(f, &u, "bindel", "", "XF86MonBrightnessDown", "exec, brightnessctl set 5%-");
	bind(f, &u, "bindel", "", "XF86AudioRaiseVolume", "exec, wpctl set-volume -l 1.5 @DEFAULT_AUDIO_SINK@ 5%+");
	bind(f, &u, "bindel", "", "XF86AudioLowerVolume", "exec, wpctl set-volume @DEFAULT_AUDIO_SINK@ 5%-");
	bind(f, &u, "bindl", "", "XF86AudioMute", "exec, wpctl set-mute @DEFAULT_AUDIO_SINK@ toggle");
	bind(f, &u, "bindl", "", "XF86AudioPlay", "exec, playerctl play-pause");
	bind(f, &u, "bind", "SUPER", "mouse_down", "workspace, e+1");
	bind(f, &u, "bind", "SUPER", "mouse_up", "workspace, e-1");
	fputs("bindm = SUPER, mouse:272, movewindow\nbindm = SUPER, mouse:273, resizewindow\n", f);
	for (int i = 1; i <= t->workspaces && i <= 10; i++) {
		char k[12], act[40];

		snprintf(k, sizeof k, "%d", i == 10 ? 0 : i);
		snprintf(act, sizeof act, "workspace, %d", i);
		bind(f, &u, "bind", "SUPER", k, act);
		snprintf(act, sizeof act, "movetoworkspace, %d", i);
		bind(f, &u, "bind", "SUPER SHIFT", k, act);
	}
	fputs("\nsubmap = resize\nbinde = , right, resizeactive, 40 0\nbinde = , left, resizeactive, -40 0\n", f);
	fputs("binde = , up, resizeactive, 0 -40\nbinde = , down, resizeactive, 0 40\n", f);
	fputs("bind = , escape, submap, reset\nbind = , Return, submap, reset\nsubmap = reset\n", f);

	n = collect(c, "keybind.", items, 64);
	if (n)
		fputs("\n# Tus atajos (keybind.*)\n", f);
	for (size_t i = 0; i < n; i++) {
		if (rros_keybind_to_hypr(items[i]->val, line, sizeof line) == 0)
			fprintf(f, "bind = %s\n", line);
		else
			fprintf(f, "# ignorado (no tiene equivalente en Hyprland): %s\n", items[i]->key);
	}
	n = collect(c, "autostart.", items, 64);
	if (n)
		fputs("\n# Programas de inicio (autostart.*)\n", f);
	for (size_t i = 0; i < n; i++)
		if (!strchr(items[i]->val, '#') && !strchr(items[i]->val, '\n'))
			fprintf(f, "exec-once = %s\n", items[i]->val);
	fputs("\n# hyprlang noerror true\nsource = ~/.config/hypr/personal.conf\n# hyprlang noerror false\n", f);
}

void rros_render_hyprlock(const struct rros_theme *t, const struct rros_palette *p,
			  const struct rros_conf *c, FILE *f)
{
	char a[24], wp[300];

	(void)c;
	rros_wallpaper_path(t, p, wp, sizeof wp);
	fprintf(f, "# %s. No lo edites.\n", RROS_GENERATED);
	fputs("general {\n    hide_cursor = true\n    grace = 0\n}\n\n", f);
	fputs("background {\n    monitor =\n", f);
	if (wp[0])
		fprintf(f, "    path = %s\n    blur_passes = %d\n    blur_size = 7\n", wp, t->animations ? 2 : 0);
	else
		fprintf(f, "    color = %s\n", rgba(a, sizeof a, p->bg, 1));
	fputs("}\n\n", f);
	fputs("label {\n    monitor =\n    text = $TIME\n", f);
	fprintf(f, "    color = %s\n    font_size = 84\n    font_family = %s\n", rgba(a, sizeof a, p->text, 1), t->font);
	fputs("    position = 0, 160\n    halign = center\n    valign = center\n}\n\n", f);
	fputs("label {\n    monitor =\n    text = cmd[update:60000] LC_TIME=es_ES.UTF-8 date +'%A, %-d de %B'\n", f);
	fprintf(f, "    color = %s\n    font_size = 20\n    font_family = %s\n", rgba(a, sizeof a, p->muted, 1), t->font);
	fputs("    position = 0, 80\n    halign = center\n    valign = center\n}\n\n", f);
	fputs("input-field {\n    monitor =\n    size = 320, 56\n", f);
	fprintf(f, "    outline_thickness = %d\n", t->border > 1 ? t->border : 2);
	fprintf(f, "    outer_color = %s\n", rgba(a, sizeof a, p->accent, 1));
	fprintf(f, "    inner_color = %s\n", rgba(a, sizeof a, p->surface, 0.9));
	fprintf(f, "    font_color = %s\n", rgba(a, sizeof a, p->text, 1));
	fprintf(f, "    check_color = %s\n", rgba(a, sizeof a, p->yellow, 1));
	fprintf(f, "    fail_color = %s\n", rgba(a, sizeof a, p->red, 1));
	fprintf(f, "    rounding = %d\n", t->radius);
	fputs("    placeholder_text = Contraseña\n    fail_text = Contraseña incorrecta\n    fade_on_empty = false\n", f);
	fputs("    position = 0, -40\n    halign = center\n    valign = center\n}\n", f);
}
