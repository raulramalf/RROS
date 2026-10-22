/* KDE Plasma: lo que Lily escribe en los archivos de configuración de KDE a partir de rros-config.
 *  - el guion de plasmashell que crea la barra superior y el dock;
 *  - las ediciones de kwinrc, kdeglobals, kcminputrc, powerdevilrc... (se mezclan con lo que ya hay);
 *  - el perfil de Konsole, los atajos propios, las reglas de ventanas y los programas de inicio. */
#include "theme.h"
#include "rros_profile.h"
#include "rros_schema.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

const char *const rros_kde_files[] = { "kdeglobals", "kwinrc", "kcminputrc", "kxkbrc", "kscreenlockerrc", "powerdevilrc",
				       "plasmaparc", "plasmarc", "baloofilerc", "ksmserverrc", "konsolerc", "kwinrulesrc",
				       "kglobalshortcutsrc", "gtk-3.0/settings.ini", "gtk-4.0/settings.ini", NULL };

const char *rros_desktoptheme(const struct rros_theme *t)
{
	return t->dock_3d ? "rros-3d" : "rros";
}

/* ---- guion de los paneles ---- */

static void js_launchers(FILE *f, const char *var)
{
	fprintf(f, "%s.writeConfig(\"launchers\", [\n"
		   "    \"applications:org.kde.dolphin.desktop\",\n"
		   "    \"applications:firefox.desktop\",\n"
		   "    \"applications:rros-apps.desktop\",\n"
		   "    \"applications:org.kde.konsole.desktop\",\n"
		   "    \"applications:org.kde.kate.desktop\",\n"
		   "    \"applications:rros-settings.desktop\"\n"
		   "]);\n", var);
}

static void js_clock(FILE *f, const struct rros_theme *t, int date)
{
	fputs("var clock = bar.addWidget(\"org.kde.plasma.digitalclock\");\n"
	      "clock.currentConfigGroup = [\"Appearance\"];\n", f);
	/* 0: 12 horas, 2: 24 horas */
	fprintf(f, "clock.writeConfig(\"use24hFormat\", %d);\n", t->clock_24h ? 2 : 0);
	if (date)
		fputs("clock.writeConfig(\"showDate\", true);\n"
		      "clock.writeConfig(\"dateDisplayFormat\", \"BesideTime\");\n"
		      "clock.writeConfig(\"dateFormat\", \"custom\");\n"
		      "clock.writeConfig(\"customDateFormat\", \"ddd d MMM\");\n", f);
	else
		fputs("clock.writeConfig(\"showDate\", false);\n", f);
}

void rros_render_plasma_layout(const struct rros_theme *t, const struct rros_palette *p, const struct rros_conf *c,
			       FILE *f)
{
	int classic = t->taskbar;
	int pill = !strcmp(t->panel_style, "pill"), floating = strcmp(t->panel_style, "flat") != 0;
	int focus = pill && !classic;   /* estilo «Enfoque»: reloj en el centro */

	(void)p; (void)c;
	fprintf(f, "// %s: barra y dock de Lily. Lo ejecuta plasmashell (rros-theme escritorio).\n", RROS_GENERATED);
	fputs("panels().forEach(function (p) { p.remove(); });\n\n", f);

	/* --- barra --- */
	fputs("var bar = new Panel;\n", f);
	fprintf(f, "bar.location = \"%s\";\n", t->panel_pos);
	fprintf(f, "bar.height = %d;\n", t->panel_size);
	fprintf(f, "bar.floating = %s;\n", floating ? "true" : "false");
	if (pill)
		fputs("bar.lengthMode = \"fit\";\nbar.alignment = \"center\";\n", f);
	fprintf(f, "bar.hiding = \"%s\";\n", t->panel_autohide ? "autohide" : "none");
	if (classic) {
		fputs("var menu = bar.addWidget(\"org.kde.plasma.kickoff\");\n"
		      "menu.currentConfigGroup = [\"General\"];\n"
		      "menu.writeConfig(\"icon\", \"rros\");\n"
		      "var tasks = bar.addWidget(\"org.kde.plasma.taskmanager\");\n"
		      "tasks.currentConfigGroup = [\"General\"];\n", f);
		js_launchers(f, "tasks");
		if (t->workspaces > 1)
			fputs("bar.addWidget(\"org.kde.plasma.pager\");\n", f);
		fputs("bar.addWidget(\"org.kde.plasma.systemtray\");\n", f);
		js_clock(f, t, 1);
		fputs("bar.addWidget(\"org.kde.plasma.showdesktop\");\n", f);
	} else {
		fputs("bar.addWidget(\"org.rros.menu\");\n", f);
		if (t->global_menu && !focus)
			fputs("bar.addWidget(\"org.kde.plasma.appmenu\");\n", f);
		if (t->workspaces > 1)
			fputs("bar.addWidget(\"org.kde.plasma.pager\");\n", f);
		fputs("bar.addWidget(\"org.kde.plasma.panelspacer\");\n", f);
		if (focus) {
			js_clock(f, t, 1);
			fputs("bar.addWidget(\"org.kde.plasma.panelspacer\");\n", f);
			fputs("bar.addWidget(\"org.kde.plasma.systemtray\");\n", f);
		} else {
			fputs("bar.addWidget(\"org.kde.plasma.systemtray\");\n", f);
			js_clock(f, t, 1);
		}
	}

	/* --- dock --- */
	if (t->dock && !classic) {
		const char *hide = !strcmp(t->dock_hiding, "never") ? "none" :
				   !strcmp(t->dock_hiding, "auto") ? "autohide" : "dodgewindows";

		fputs("\nvar dock = new Panel;\n", f);
		fprintf(f, "dock.location = \"%s\";\n", t->dock_pos);
		/* el icono, su margen y, en 3D, la base del estante */
		fprintf(f, "dock.height = %d;\n", t->dock_icon + (t->dock_3d ? 22 : 16));
		fputs("dock.floating = true;\ndock.lengthMode = \"fit\";\ndock.alignment = \"center\";\n", f);
		fprintf(f, "dock.hiding = \"%s\";\n", hide);
		if (t->dock_launchpad)
			fputs("var pad = dock.addWidget(\"org.kde.plasma.kickerdash\");\n"
			      "pad.currentConfigGroup = [\"General\"];\n"
			      "pad.writeConfig(\"icon\", \"rros-launchpad\");\n"
			      /* la tecla Super abre el lanzador que tiene Alt+F1: el Launchpad */
			      "pad.globalShortcut = \"Alt+F1\";\n", f);
		fputs("var dt = dock.addWidget(\"org.kde.plasma.icontasks\");\n"
		      "dt.currentConfigGroup = [\"General\"];\n", f);
		js_launchers(f, "dt");
		fputs("dt.writeConfig(\"showOnlyCurrentDesktop\", false);\n"
		      "dt.writeConfig(\"groupingStrategy\", 1);\n"
		      "dt.writeConfig(\"indicateAudioStreams\", true);\n"
		      "dt.writeConfig(\"iconSpacing\", 1);\n"
		      "dock.addWidget(\"org.kde.plasma.marginsseparator\");\n"
		      "dock.addWidget(\"org.kde.plasma.trash\");\n", f);
	}
}

/* ---- ediciones de los archivos de KDE ---- */

struct edits {
	struct rros_ini_edit *e;
	size_t n, max;
};

static void put(struct edits *s, const char *file, const char *group, const char *key, const char *fmt, ...)
	__attribute__((format(printf, 5, 6)));

static void put(struct edits *s, const char *file, const char *group, const char *key, const char *fmt, ...)
{
	struct rros_ini_edit *e;
	va_list ap;

	if (s->n >= s->max)
		return;
	e = &s->e[s->n++];
	memset(e, 0, sizeof *e);
	e->file = file;
	snprintf(e->group, sizeof e->group, "%s", group);
	e->key = key;
	va_start(ap, fmt);
	vsnprintf(e->value, sizeof e->value, fmt, ap);
	va_end(ap);
}

static void del_group(struct edits *s, const char *file, const char *group)
{
	if (s->n >= s->max)
		return;
	memset(&s->e[s->n], 0, sizeof s->e[s->n]);
	s->e[s->n].file = file;
	snprintf(s->e[s->n].group, sizeof s->e[s->n].group, "%s", group);
	s->e[s->n].del_group = 1;
	s->n++;
}

static const char *tf(int b)
{
	return b ? "true" : "false";
}

static void font(char *out, size_t n, const char *family, int size)
{
	snprintf(out, n, "%s,%d,-1,5,400,0,0,0,0,0,0,0,0,0,0,1", family, size < 6 ? 6 : size);
}

/* nivel de efectos: 0 ninguno, 1 básico, 2 avanzado */
static int effects_level(const struct rros_theme *t)
{
	if (t->soft_render || !strcmp(t->effects, "off"))
		return 0;
	return !strcmp(t->effects, "advanced") ? 2 : 1;
}

static void kwin(struct edits *s, const struct rros_theme *t)
{
	int lvl = effects_level(t), anim = t->animations && lvl > 0;
	const char *st = t->animation_style;
	int open_glide = !strcmp(st, "deslizar") || !strcmp(st, "rebote");
	int lamp = lvl == 2 && strcmp(st, "rapida") != 0;
	/* lo que cuesta GPU (desenfoque, lámpara mágica...), solo en el nivel avanzado */
	put(s, "kwinrc", "Plugins", "blurEnabled", "%s", tf(lvl == 2 && t->blur));
	put(s, "kwinrc", "Plugins", "contrastEnabled", "%s", tf(lvl == 2 && t->blur));
	put(s, "kwinrc", "Plugins", "wobblywindowsEnabled", "false");
	put(s, "kwinrc", "Plugins", "magiclampEnabled", "%s", tf(anim && lamp));
	put(s, "kwinrc", "Plugins", "translucencyEnabled", "%s", tf(lvl > 0 && t->transparency > 0));
	put(s, "kwinrc", "Plugins", "squashEnabled", "%s", tf(anim && !lamp));
	put(s, "kwinrc", "Plugins", "scaleEnabled", "%s", tf(anim && !open_glide));
	put(s, "kwinrc", "Plugins", "glideEnabled", "%s", tf(anim && open_glide));
	put(s, "kwinrc", "Plugins", "fadingpopupsEnabled", "%s", tf(anim));
	put(s, "kwinrc", "Plugins", "slidingpopupsEnabled", "%s", tf(anim));
	put(s, "kwinrc", "Plugins", "slideEnabled", "%s", tf(anim));
	put(s, "kwinrc", "Plugins", "maximizeEnabled", "%s", tf(anim && lvl == 2));
	put(s, "kwinrc", "Plugins", "loginEnabled", "%s", tf(anim));
	put(s, "kwinrc", "Plugins", "logoutEnabled", "%s", tf(anim));
	put(s, "kwinrc", "Plugins", "frozenappEnabled", "%s", tf(lvl > 0));
	put(s, "kwinrc", "Plugins", "diminactiveEnabled", "%s", tf(lvl > 0 && t->dim_inactive));
	put(s, "kwinrc", "Plugins", "dimscreenEnabled", "%s", tf(lvl > 0));
	if (lvl == 2 && t->blur) {
		int b = t->blur_strength > 15 ? 15 : t->blur_strength;

		put(s, "kwinrc", "Effect-blur", "BlurStrength", "%d", b);
		put(s, "kwinrc", "Effect-blur", "NoiseStrength", "%d", 2);
	}
	if (t->transparency > 0) {
		put(s, "kwinrc", "Effect-translucency", "Inactive", "%d", 100 - t->transparency / 2);
		put(s, "kwinrc", "Effect-translucency", "MoveResize", "%d", 100 - t->transparency);
	}
	if (t->dim_inactive)
		put(s, "kwinrc", "Effect-diminactive", "Strength", "%d", 20);
	/* esquina activa arriba a la izquierda: vista general (7 = TopLeft, 9 = ninguna) */
	put(s, "kwinrc", "Effect-overview", "BorderActivate", "%d", t->hot_corners ? 7 : 9);
	put(s, "kwinrc", "Desktops", "Number", "%d", t->workspaces);
	put(s, "kwinrc", "Desktops", "Rows", "%d", 1);
	put(s, "kwinrc", "NightColor", "Active", "%s", tf(strcmp(t->night_light, "off") != 0));
	put(s, "kwinrc", "NightColor", "Mode", "%s", !strcmp(t->night_light, "on") ? "Constant" : "Automatic");
	put(s, "kwinrc", "NightColor", "NightTemperature", "%d", t->night_temp);
	put(s, "kwinrc", "org.kde.kdecoration2", "library", "org.kde.kwin.aurorae");
	put(s, "kwinrc", "org.kde.kdecoration2", "theme", "__aurorae__svg__RROS");
	/* X cerrar, I minimizar, A maximizar; M menú de la ventana */
	put(s, "kwinrc", "org.kde.kdecoration2", "ButtonsOnLeft", "%s", t->buttons_left ? "XIA" : "M");
	put(s, "kwinrc", "org.kde.kdecoration2", "ButtonsOnRight", "%s", t->buttons_left ? "" : "IAX");
	put(s, "kwinrc", "org.kde.kdecoration2", "BorderSize", "None");
	put(s, "kwinrc", "org.kde.kdecoration2", "BorderSizeAuto", "false");
	put(s, "kwinrc", "Wayland", "VirtualKeyboardEnabled", "%s", tf(t->osk));
	/* el teclado en pantalla (Maliit) solo se arranca si se usa: ahorra memoria */
	put(s, "kwinrc", "Wayland", "InputMethod", "%s", t->osk ? "/usr/share/applications/com.github.maliit.keyboard.desktop" : "");
	put(s, "kwinrc", "Xwayland", "Scale", "%.2f", t->scale / 100.0);
	/* gestos del panel táctil: los de KWin, con 3 dedos para cambiar de escritorio y 4 para la vista general */
	put(s, "kwinrc", "Effect-slide", "SlideBackground", "%s", "false");
}

static void kdeglobals(struct edits *s, const struct rros_theme *t, const struct rros_palette *p)
{
	char fb[160];
	int r, g, b;
	double factor;

	font(fb, sizeof fb, t->font, t->font_size);
	put(s, "kdeglobals", "General", "font", "%s", fb);
	put(s, "kdeglobals", "General", "menuFont", "%s", fb);
	font(fb, sizeof fb, t->font, t->font_size - 1);
	put(s, "kdeglobals", "General", "toolBarFont", "%s", fb);
	font(fb, sizeof fb, t->font, t->font_size - 2);
	put(s, "kdeglobals", "General", "smallestReadableFont", "%s", fb);
	font(fb, sizeof fb, t->font_mono, t->font_size);
	put(s, "kdeglobals", "General", "fixed", "%s", fb);
	font(fb, sizeof fb, t->font, t->font_size);
	put(s, "kdeglobals", "WM", "activeFont", "%s", fb);
	put(s, "kdeglobals", "General", "XftAntialias", "%s", tf(strcmp(t->font_aa, "none") != 0));
	put(s, "kdeglobals", "General", "XftSubPixel", "%s", !strcmp(t->font_aa, "rgba") ? "rgb" : "none");
	put(s, "kdeglobals", "General", "XftHintStyle", "hint%s", t->font_hinting);
	rros_hex_rgb(p->accent, &r, &g, &b);
	put(s, "kdeglobals", "General", "AccentColor", "%d,%d,%d", r, g, b);
	/* iconos: Breeze sigue el modo claro u oscuro; los logotipos de las aplicaciones son los suyos */
	if (!strcmp(t->icon_theme, "breeze") || !strcmp(t->icon_theme, "breeze-dark") || !strcmp(t->icon_theme, "Adwaita"))
		put(s, "kdeglobals", "Icons", "Theme", "%s", t->dark ? "breeze-dark" : "breeze");
	else
		put(s, "kdeglobals", "Icons", "Theme", "%s", t->icon_theme);
	/* duración de las animaciones: 0 las quita; la velocidad las acorta o alarga */
	factor = (!t->animations || effects_level(t) == 0) ? 0.0 : 0.7 * 100.0 / (t->animation_speed ? t->animation_speed : 100);
	put(s, "kdeglobals", "KDE", "AnimationDurationFactor", "%.3g", factor);
	put(s, "kdeglobals", "KDE", "LookAndFeelPackage", "org.rros.desktop");
	put(s, "kdeglobals", "KDE", "SingleClick", "false");
	put(s, "kdeglobals", "Sounds", "Enable", "%s", tf(t->event_sounds));
	put(s, "kdeglobals", "Sounds", "Theme", "%s", t->sound_theme);
	put(s, "kdeglobals", "General", "TerminalApplication", "%s", t->app_term);
	put(s, "kdeglobals", "General", "TerminalService", "%s", !strcmp(t->app_term, "konsole") ? "org.kde.konsole.desktop" : "");
}

static void input(struct edits *s, const struct rros_theme *t)
{
	/* velocidad del puntero de KDE: -1 a 1 */
	double accel = t->pointer_speed / 100.0;

	put(s, "kcminputrc", "Mouse", "cursorTheme", "%s", t->cursor_theme);
	put(s, "kcminputrc", "Mouse", "cursorSize", "%d", t->cursor_size);
	/* aplicaciones GTK (Firefox, GIMP…): el mismo puntero */
	put(s, "gtk-3.0/settings.ini", "Settings", "gtk-cursor-theme-name", "%s", t->cursor_theme);
	put(s, "gtk-3.0/settings.ini", "Settings", "gtk-cursor-theme-size", "%d", t->cursor_size);
	put(s, "gtk-4.0/settings.ini", "Settings", "gtk-cursor-theme-name", "%s", t->cursor_theme);
	put(s, "gtk-4.0/settings.ini", "Settings", "gtk-cursor-theme-size", "%d", t->cursor_size);
	put(s, "kcminputrc", "Keyboard", "RepeatDelay", "%d", t->repeat_delay);
	put(s, "kcminputrc", "Keyboard", "RepeatRate", "%d", t->repeat_rate);
	/* 0 encendido, 1 apagado */
	put(s, "kcminputrc", "Keyboard", "NumLock", "%d", t->numlock ? 0 : 1);
	/* valores por defecto para todos los ratones y paneles táctiles (los de cada aparato los cambia Lily Ajustes) */
	put(s, "kcminputrc", "Libinput][Defaults][Touchpad", "TapToClick", "%s", tf(t->tap_to_click));
	put(s, "kcminputrc", "Libinput][Defaults][Touchpad", "NaturalScroll", "%s", tf(t->natural_scroll));
	put(s, "kcminputrc", "Libinput][Defaults][Touchpad", "DisableWhileTyping", "%s", tf(t->dwt));
	put(s, "kcminputrc", "Libinput][Defaults][Touchpad", "PointerAcceleration", "%.2f", accel);
	put(s, "kcminputrc", "Libinput][Defaults][Pointer", "PointerAcceleration", "%.2f", accel);
	put(s, "kxkbrc", "Layout", "LayoutList", "%s", t->keymap);
	put(s, "kxkbrc", "Layout", "Use", "true");
}

static int lid_code(const char *a)
{
	/* acciones de PowerDevil: 0 nada, 1 suspender, 2 hibernar, 8 apagar */
	return !strcmp(a, "ignore") ? 0 : !strcmp(a, "hibernate") ? 2 : !strcmp(a, "poweroff") ? 8 : 1;
}

static void power(struct edits *s, const struct rros_theme *t)
{
	static const char *const states[] = { "AC", "Battery", "LowBattery" };

	for (int i = 0; i < 3; i++) {
		char g[40];
		int off = t->screen_off_min, sus = t->suspend_min;

		if (i > 0 && off > 5)
			off = i == 1 ? (off > 10 ? 10 : off) : 2;   /* con batería, antes */
		if (i > 0 && sus > 15)
			sus = i == 1 ? 15 : 5;
		snprintf(g, sizeof g, "%s][Display", states[i]);
		put(s, "powerdevilrc", g, "TurnOffDisplayWhenIdle", "%s", tf(off > 0));
		put(s, "powerdevilrc", g, "TurnOffDisplayIdleTimeoutSec", "%d", off > 0 ? off * 60 : 600);
		put(s, "powerdevilrc", g, "DimDisplayWhenIdle", "%s", tf(off > 1));
		snprintf(g, sizeof g, "%s][SuspendAndShutdown", states[i]);
		put(s, "powerdevilrc", g, "AutoSuspendAction", "%d", sus > 0 ? 1 : 0);
		put(s, "powerdevilrc", g, "AutoSuspendIdleTimeoutSec", "%d", sus > 0 ? sus * 60 : 1800);
		put(s, "powerdevilrc", g, "LidAction", "%d", lid_code(t->lid_action));
	}
	put(s, "powerdevilrc", "BatteryManagement", "BatteryLowLevel", "%d", t->battery_low);
	put(s, "powerdevilrc", "BatteryManagement", "BatteryCriticalLevel", "%d", t->battery_low > 6 ? 5 : 2);
	put(s, "kscreenlockerrc", "Daemon", "Autolock", "%s", tf(t->lock_min > 0));
	put(s, "kscreenlockerrc", "Daemon", "Timeout", "%d", t->lock_min > 0 ? t->lock_min : 10);
	put(s, "kscreenlockerrc", "Daemon", "LockOnResume", "%s", tf(t->lock_min > 0));
	put(s, "kscreenlockerrc", "Greeter", "WallpaperPlugin", "org.kde.image");
}

static void other(struct edits *s, const struct rros_theme *t)
{
	put(s, "plasmaparc", "General", "RaiseMaximumVolume", "%s", tf(t->max_volume > 100));
	put(s, "plasmaparc", "General", "VolumeStep", "%d", t->volume_step);
	put(s, "plasmaparc", "General", "AudioFeedback", "%s", tf(t->volume_feedback));
	put(s, "plasmarc", "Theme", "name", "%s", rros_desktoptheme(t));
	/* el indexador de archivos (Baloo) consume disco y CPU al principio: solo si se pide */
	put(s, "baloofilerc", "Basic Settings", "Indexing-Enabled", "%s", tf(t->indexer));
	/* cada sesión empieza limpia: el inicio es más rápido */
	put(s, "ksmserverrc", "General", "loginMode", "emptySession");
	put(s, "ksmserverrc", "General", "confirmLogout", "false");
	put(s, "konsolerc", "Desktop Entry", "DefaultProfile", "Lily.profile");
}

static void rules(struct edits *s, const struct rros_conf *c)
{
	char list[2048] = "";
	size_t used = 0;
	int count = 0;

	/* las reglas de Lily se llaman rros-ID; las demás (las que haga el usuario en KDE) no se tocan */
	del_group(s, "kwinrulesrc", "rros-*");
	for (size_t i = 0; i < c->n; i++) {
		const struct rros_kv *kv = &c->items[i];
		struct rros_rule r;
		char g[64];

		if (strncmp(kv->key, "rule.", 5) || rros_rule_parse(kv->val, &r))
			continue;
		snprintf(g, sizeof g, "rros-%.40s", kv->key + 5);
		put(s, "kwinrulesrc", g, "Description", "Lily: %s", kv->key + 5);
		if (r.by_title) {
			put(s, "kwinrulesrc", g, "title", "%s", r.match);
			put(s, "kwinrulesrc", g, "titlematch", "2");
		} else {
			put(s, "kwinrulesrc", g, "wmclass", "%s", r.match);
			put(s, "kwinrulesrc", g, "wmclassmatch", "2");
		}
		/* 2: forzar; 3: aplicar al abrir */
		if (!strcmp(r.action, "opacity")) {
			put(s, "kwinrulesrc", g, "opacityactive", "%d", r.opacity);
			put(s, "kwinrulesrc", g, "opacityactiverule", "2");
			put(s, "kwinrulesrc", g, "opacityinactive", "%d", r.opacity);
			put(s, "kwinrulesrc", g, "opacityinactiverule", "2");
		} else if (!strcmp(r.action, "maximized")) {
			put(s, "kwinrulesrc", g, "maximizehoriz", "true");
			put(s, "kwinrulesrc", g, "maximizehorizrule", "3");
			put(s, "kwinrulesrc", g, "maximizevert", "true");
			put(s, "kwinrulesrc", g, "maximizevertrule", "3");
		} else {
			static const char *const acts[][2] = { { "above", "aboverule" }, { "fullscreen", "fullscreenrule" },
							       { "noborder", "noborderrule" }, { "skiptaskbar", "skiptaskbarrule" },
							       { "minimized", "minimizerule" } };

			for (size_t a = 0; a < sizeof acts / sizeof *acts; a++)
				if (!strcmp(r.action, acts[a][0])) {
					put(s, "kwinrulesrc", g, !strcmp(acts[a][0], "minimized") ? "minimize" : acts[a][0], "true");
					put(s, "kwinrulesrc", g, acts[a][1], "%s", a == 1 || a == 4 ? "3" : "2");
				}
		}
		if (used + strlen(g) + 2 < sizeof list)
			used += (size_t)snprintf(list + used, sizeof list - used, "%s%s", used ? "," : "", g);
		count++;
	}
	/* la lista de reglas de KWin conserva las del usuario (rros_ini_apply mezcla «rules» y recalcula «count») */
	put(s, "kwinrulesrc", "General", "rules", "%s", list);
	s->e[s->n - 1].merge_list = 1;
	(void)count;
}

/* atajos propios: cada uno es un lanzador oculto con su combinación en kglobalshortcutsrc */
static void shortcuts(struct edits *s, const struct rros_conf *c)
{
	del_group(s, "kglobalshortcutsrc", "services][rros-kb-*");
	for (size_t i = 0; i < c->n; i++) {
		const struct rros_kv *kv = &c->items[i];
		char combo[96], cmd[256], g[96];

		if (strncmp(kv->key, "keybind.", 8) || rros_keybind_parse(kv->val, combo, sizeof combo, cmd, sizeof cmd))
			continue;
		snprintf(g, sizeof g, "services][rros-kb-%.40s.desktop", kv->key + 8);
		put(s, "kglobalshortcutsrc", g, "_launch", "%s", combo);
	}
}

size_t rros_plasma_edits(const struct rros_theme *t, const struct rros_palette *p, const struct rros_conf *c,
			 struct rros_ini_edit *out, size_t max)
{
	struct edits s = { out, 0, max };

	kwin(&s, t);
	kdeglobals(&s, t, p);
	input(&s, t);
	power(&s, t);
	other(&s, t);
	rules(&s, c);
	shortcuts(&s, c);
	return s.n;
}

/* ---- archivos INI de KDE: leer, cambiar claves y guardar sin perder lo demás ---- */

struct line {
	char *text;
};

static int glob_match(const char *pat, const char *s)
{
	size_t n = strlen(pat);

	if (n && pat[n - 1] == '*')
		return !strncmp(pat, s, n - 1);
	return !strcmp(pat, s);
}

/* nombre del grupo de una línea "[A][B]" -> "A][B"; NULL si no es cabecera */
static int header_of(const char *l, char *g, size_t n)
{
	size_t len = strlen(l);

	while (len && (l[len - 1] == '\n' || l[len - 1] == '\r' || l[len - 1] == ' '))
		len--;
	if (len < 2 || l[0] != '[' || l[len - 1] != ']')
		return 0;
	if (len - 2 >= n)
		return 0;
	memcpy(g, l + 1, len - 2);
	g[len - 2] = '\0';
	return 1;
}

static int key_is(const char *l, const char *key)
{
	size_t k = strlen(key);

	if (strncmp(l, key, k))
		return 0;
	/* clave=valor, o clave[$e]=valor */
	return l[k] == '=' || l[k] == '[';
}

static char **read_lines(const char *path, size_t *n)
{
	FILE *f = fopen(path, "r");
	char **v = NULL, buf[4096];
	size_t cap = 0;

	*n = 0;
	if (!f)
		return NULL;
	while (fgets(buf, sizeof buf, f)) {
		size_t len = strlen(buf);

		if (len && buf[len - 1] == '\n')
			buf[--len] = '\0';
		if (*n + 1 >= cap) {
			char **nv = realloc(v, (cap = cap ? cap * 2 : 64) * sizeof *v);

			if (!nv)
				break;
			v = nv;
		}
		v[(*n)++] = strdup(buf);
	}
	fclose(f);
	return v;
}

int rros_ini_get(const char *path, const char *group, const char *key, char *out, size_t n)
{
	size_t nl;
	char **v = read_lines(path, &nl), g[128];
	int in = 0, found = -1;

	out[0] = '\0';
	for (size_t i = 0; i < nl; i++) {
		if (header_of(v[i], g, sizeof g))
			in = !strcmp(g, group);
		else if (in && found < 0 && key_is(v[i], key) && strchr(v[i], '='))
			found = snprintf(out, n, "%s", strchr(v[i], '=') + 1) >= 0 ? 0 : -1;
		free(v[i]);
	}
	free(v);
	return found;
}

/* pone «key=value» en el grupo: cambia la línea si ya está; si no, la añade al final del grupo o en un grupo nuevo */
static int set_line(char ***pv, size_t *pnl, size_t *pcap, const char *group, const char *key, const char *line)
{
	char **v = *pv, g[128];
	size_t nl = *pnl, gi = (size_t)-1, last = (size_t)-1;
	int in = 0;

	for (size_t i = 0; i < nl; i++) {
		if (!v[i])
			continue;
		if (header_of(v[i], g, sizeof g)) {
			in = !strcmp(g, group);
			if (in)
				gi = last = i;
			continue;
		}
		if (!in)
			continue;
		if (v[i][0])
			last = i;
		if (key_is(v[i], key)) {
			free(v[i]);
			v[i] = strdup(line);
			return 0;
		}
	}
	if (nl + 3 >= *pcap) {
		char **nv = realloc(v, (*pcap = (nl + 3) * 2) * sizeof *v);

		if (!nv)
			return -1;
		*pv = v = nv;
	}
	if (gi == (size_t)-1) {
		char h[140];

		if (nl && v[nl - 1] && v[nl - 1][0])
			v[nl++] = strdup("");
		snprintf(h, sizeof h, "[%s]", group);
		v[nl++] = strdup(h);
		v[nl++] = strdup(line);
	} else {
		memmove(v + last + 2, v + last + 1, (nl - last - 1) * sizeof *v);
		v[last + 1] = strdup(line);
		nl++;
	}
	*pnl = nl;
	return 0;
}

/* lista separada por comas (las reglas de KWin): se quedan las entradas que no son rros-* y se añaden las nuevas */
static int merge_list(char ***pv, size_t *pnl, size_t *pcap, const char *file, const struct rros_ini_edit *e)
{
	char old[1024] = "", merged[2048] = "", ours[sizeof e->value], *tok, *save = NULL, line[2200], g[128];
	size_t mu = 0;
	int cnt = 0, in = 0;

	for (size_t i = 0; i < *pnl; i++) {
		const char *l = (*pv)[i];

		if (!l)
			continue;
		if (header_of(l, g, sizeof g))
			in = !strcmp(g, e->group);
		else if (in && key_is(l, e->key) && strchr(l, '='))
			snprintf(old, sizeof old, "%s", strchr(l, '=') + 1);
	}
	for (tok = strtok_r(old, ",", &save); tok; tok = strtok_r(NULL, ",", &save))
		if (strncmp(tok, "rros-", 5) && mu + strlen(tok) + 2 < sizeof merged) {
			mu += (size_t)snprintf(merged + mu, sizeof merged - mu, "%s%s", mu ? "," : "", tok);
			cnt++;
		}
	snprintf(ours, sizeof ours, "%s", e->value);
	for (tok = strtok_r(ours, ",", &save); tok; tok = strtok_r(NULL, ",", &save))
		if (mu + strlen(tok) + 2 < sizeof merged) {
			mu += (size_t)snprintf(merged + mu, sizeof merged - mu, "%s%s", mu ? "," : "", tok);
			cnt++;
		}
	snprintf(line, sizeof line, "%s=%s", e->key, merged);
	if (set_line(pv, pnl, pcap, e->group, e->key, line))
		return -1;
	if (!strcmp(file, "kwinrulesrc")) {
		snprintf(line, sizeof line, "count=%d", cnt);
		return set_line(pv, pnl, pcap, e->group, "count", line);
	}
	return 0;
}

int rros_ini_apply(const char *path, const char *file, const struct rros_ini_edit *e, size_t ne)
{
	size_t nl = 0, cap;
	char **v = read_lines(path, &nl), g[128], tmp[1100];
	FILE *f;
	int bad = 0;

	cap = nl;
	/* 1) grupos que se borran */
	for (size_t k = 0; k < ne; k++) {
		int in = 0;

		if (strcmp(e[k].file, file) || !e[k].del_group)
			continue;
		for (size_t i = 0; i < nl; i++) {
			if (!v[i])
				continue;
			if (header_of(v[i], g, sizeof g))
				in = glob_match(e[k].group, g);
			if (in) {
				free(v[i]);
				v[i] = NULL;
			}
		}
	}
	/* 2) claves */
	for (size_t k = 0; k < ne && !bad; k++) {
		char line[400];

		if (strcmp(e[k].file, file) || e[k].del_group || !e[k].key)
			continue;
		if (e[k].merge_list) {
			bad = merge_list(&v, &nl, &cap, file, &e[k]) != 0;
		} else {
			snprintf(line, sizeof line, "%s=%s", e[k].key, e[k].value);
			bad = set_line(&v, &nl, &cap, e[k].group, e[k].key, line) != 0;
		}
	}
	snprintf(tmp, sizeof tmp, "%s.rros-tmp", path);
	if (bad || rros_mkdir_parents(path) || !(f = fopen(tmp, "w"))) {
		for (size_t i = 0; i < nl; i++)
			free(v[i]);
		free(v);
		return -1;
	}
	{
		int prev_blank = 1;

		for (size_t i = 0; i < nl; i++) {
			if (!v[i])
				continue;
			/* sin líneas en blanco repetidas (quedan al borrar grupos) */
			if (v[i][0] || !prev_blank)
				fprintf(f, "%s\n", v[i]);
			prev_blank = !v[i][0];
			free(v[i]);
		}
	}
	free(v);
	if (fclose(f) != 0 || rename(tmp, path) != 0) {
		remove(tmp);
		return -1;
	}
	return 0;
}

/* ---- Konsole ---- */

void rros_render_konsole_profile(const struct rros_theme *t, const struct rros_palette *p, const struct rros_conf *c,
				 FILE *f)
{
	(void)p; (void)c;
	fprintf(f, "# %s. Cambia la terminal en Lily Ajustes o con «rros config set term.<clave>».\n", RROS_GENERATED);
	fprintf(f, "[Appearance]\nColorScheme=Lily\nFont=%s,%d,-1,5,400,0,0,0,0,0,0,0,0,0,0,1\n\n", t->font_mono,
		t->font_size + 1);
	/* forma del cursor: 0 bloque, 1 barra, 2 subrayado */
	fprintf(f, "[Cursor Options]\nCursorShape=%d\n\n",
		!strcmp(t->term_cursor, "beam") ? 1 : !strcmp(t->term_cursor, "underline") ? 2 : 0);
	fprintf(f, "[General]\nName=Lily\nParent=FALLBACK/\nTerminalMargin=%d\nTerminalCenter=false\n\n", t->term_padding);
	fprintf(f, "[Scrolling]\nHistoryMode=%d\nHistorySize=%d\n", t->term_scrollback ? 1 : 0, t->term_scrollback);
}

static void kcolor(FILE *f, const char *name, const char *hex)
{
	int r, g, b;

	rros_hex_rgb(hex, &r, &g, &b);
	fprintf(f, "[%s]\nColor=%d,%d,%d\n\n", name, r, g, b);
}

void rros_render_konsole_colors(const struct rros_theme *t, const struct rros_palette *p, const struct rros_conf *c,
				FILE *f)
{
	const char *base[8] = { p->surface, p->red, p->green, p->yellow, p->blue, p->magenta, p->cyan, p->text };

	(void)c;
	fprintf(f, "# %s\n", RROS_GENERATED);
	kcolor(f, "Background", p->bg);
	kcolor(f, "BackgroundIntense", p->surface);
	kcolor(f, "Foreground", p->text);
	kcolor(f, "ForegroundIntense", p->text);
	for (int i = 0; i < 8; i++) {
		char n[24];

		snprintf(n, sizeof n, "Color%d", i);
		kcolor(f, n, i == 0 && !t->dark ? p->muted : base[i]);
		snprintf(n, sizeof n, "Color%dIntense", i);
		kcolor(f, n, i == 0 ? p->muted : base[i]);
	}
	fprintf(f, "[General]\nDescription=Lily (%s)\nOpacity=%.2f\nBlur=%s\n", t->theme,
		1.0 - t->transparency / 200.0, tf(t->blur && t->transparency > 0));
}

/* ---- lanzadores de los atajos y programas de inicio ---- */

static int write_desktop(const char *path, const char *name, const char *cmd, int hidden)
{
	FILE *f = fopen(path, "w");

	if (!f)
		return -1;
	fprintf(f, "[Desktop Entry]\n# %s\nType=Application\nName=%s\nExec=%s\n%s", RROS_GENERATED, name, cmd,
		hidden ? "NoDisplay=true\n" : "");
	return fclose(f);
}

static void clean_dir(const char *dir, const char *prefix)
{
	DIR *d = opendir(dir);
	struct dirent *e;

	if (!d)
		return;
	while ((e = readdir(d)))
		if (!strncmp(e->d_name, prefix, strlen(prefix))) {
			char p[1200];

			snprintf(p, sizeof p, "%s/%s", dir, e->d_name);
			unlink(p);
		}
	closedir(d);
}

int rros_write_launchers(const struct rros_conf *c, const char *apps_dir, const char *autostart_dir)
{
	int bad = 0;

	clean_dir(apps_dir, "rros-kb-");
	clean_dir(autostart_dir, "rros-auto-");
	for (size_t i = 0; i < c->n; i++) {
		const struct rros_kv *kv = &c->items[i];
		char path[1200], combo[96], cmd[256], name[96];

		if (!strncmp(kv->key, "keybind.", 8) &&
		    rros_keybind_parse(kv->val, combo, sizeof combo, cmd, sizeof cmd) == 0) {
			snprintf(path, sizeof path, "%s/rros-kb-%.40s.desktop", apps_dir, kv->key + 8);
			snprintf(name, sizeof name, "Atajo de Lily: %.40s", kv->key + 8);
			bad |= rros_mkdir_parents(path) || write_desktop(path, name, cmd, 1);
		} else if (!strncmp(kv->key, "autostart.", 10) && kv->val[0]) {
			snprintf(path, sizeof path, "%s/rros-auto-%.40s.desktop", autostart_dir, kv->key + 10);
			snprintf(name, sizeof name, "%.40s", kv->key + 10);
			bad |= rros_mkdir_parents(path) || write_desktop(path, name, kv->val, 0);
		}
	}
	return bad ? -1 : 0;
}
