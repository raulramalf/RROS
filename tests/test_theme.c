#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <strings.h>

#include "check.h"
#include "theme.h"
#include "rros_schema.h"

static struct rros_conf cur;
static struct rros_ini_edit ed[RROS_MAX_EDITS];

/* Renderiza a una cadena (open_memstream). El llamador libera. */
static char *render(rros_render_fn fn, const struct rros_theme *t)
{
	struct rros_palette p;
	char *buf = NULL;
	size_t len = 0;
	FILE *f = open_memstream(&buf, &len);

	rros_theme_palette(t, &p);
	fn(t, &p, &cur, f);
	fclose(f);
	return buf;
}

static void from_text(struct rros_theme *t, const char *text)
{
	rros_conf_free(&cur);
	rros_conf_init(&cur);
	CHECK(rros_conf_parse(&cur, text, NULL) == 0);
	rros_theme_from_conf(t, &cur);
}

static size_t edits(const struct rros_theme *t)
{
	struct rros_palette p;

	rros_theme_palette(t, &p);
	return rros_plasma_edits(t, &p, &cur, ed, RROS_MAX_EDITS);
}

/* valor de una edición ("" si no está) */
static const char *ev(size_t n, const char *file, const char *group, const char *key)
{
	for (size_t i = 0; i < n; i++)
		if (!ed[i].del_group && !strcmp(ed[i].file, file) && !strcmp(ed[i].group, group) && ed[i].key &&
		    !strcmp(ed[i].key, key))
			return ed[i].value;
	return "";
}

static char *slurp(const char *path)
{
	FILE *f = fopen(path, "r");
	char *s;
	long len;

	if (!f)
		return NULL;
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	rewind(f);
	s = calloc(1, (size_t)len + 1);
	if (s && fread(s, 1, (size_t)len, f) != (size_t)len) {
		free(s);
		s = NULL;
	}
	fclose(f);
	return s;
}

int main(void)
{
	struct rros_theme t;
	char *s;
	size_t n;

	/* valores por defecto: tipo macOS, dock plano, botones a la izquierda */
	from_text(&t, "");
	CHECK(t.dark == 1);
	CHECK_STR(t.accent, "");
	CHECK_STR(t.theme, "lirio");
	CHECK(t.dock == 1 && t.dock_3d == 0 && t.buttons_left == 1 && t.global_menu == 1);
	CHECK_STR(t.dock_pos, "bottom");
	CHECK_STR(rros_desktoptheme(&t), "rros");

	/* valores peligrosos se descartan; fuera de rango se acotan */
	from_text(&t, "[ui]\nfont = \"Inter\\nExec=x\"\ncursor_theme = \"a;b\"\ndock_icon_size = 500\npanel_size = 1\n");
	CHECK_STR(t.font, "Inter");
	CHECK_STR(t.cursor_theme, "breeze_cursors");
	CHECK(t.dock_icon == 96 && t.panel_size == 16);

	/* ---- temas incluidos ---- */
	from_text(&t, "[ui]\ntheme = \"nord\"\n");
	s = render(rros_render_konsole_colors, &t);
	CHECK(strstr(s, "[Background]\nColor=46,52,64\n") != NULL);   /* fondo de Nord */
	free(s);
	from_text(&t, "[ui]\ntheme = \"nord\"\naccent = \"#ff0000\"\n");
	n = edits(&t);
	CHECK_STR(ev(n, "kdeglobals", "General", "AccentColor"), "255,0,0");   /* el usuario manda */
	from_text(&t, "[ui]\ntheme = \"gruvbox\"\ncolor_scheme = \"light\"\n");
	s = render(rros_render_konsole_colors, &t);
	CHECK(strstr(s, "[Background]\nColor=251,241,199\n") != NULL);
	free(s);
	n = edits(&t);
	CHECK_STR(ev(n, "kdeglobals", "Icons", "Theme"), "breeze");          /* modo claro: iconos claros */
	{
		struct rros_palette p;

		from_text(&t, "[ui]\ntheme = \"no-existe\"\n");
		CHECK(rros_theme_palette(&t, &p) == 1); /* cae a lirio */
		CHECK_STR(p.bg, "#140609");
		from_text(&t, "[ui]\ntheme = \"../../etc/passwd\"\n");
		CHECK_STR(t.theme, "lirio"); /* nombre inválido: nunca se usa como ruta */
	}
	{
		struct rros_theme_info list[64];
		size_t k = rros_theme_list(list, 64);

		CHECK(k >= 7);
		CHECK(list[0].builtin == 1);
	}

	/* ---- efectos: avanzado, básico y sin aceleración gráfica ---- */
	from_text(&t, "[ui]\neffects = \"advanced\"\nblur = true\nblur_strength = 9\nanimation_style = \"suave\"\n");
	n = edits(&t);
	CHECK_STR(ev(n, "kwinrc", "Plugins", "blurEnabled"), "true");
	CHECK_STR(ev(n, "kwinrc", "Plugins", "magiclampEnabled"), "true");
	CHECK_STR(ev(n, "kwinrc", "Effect-blur", "BlurStrength"), "9");
	CHECK_STR(ev(n, "kdeglobals", "KDE", "AnimationDurationFactor"), "0.7");
	from_text(&t, "[ui]\neffects = \"basic\"\nblur = true\nanimation_speed = 200\n");
	n = edits(&t);
	CHECK_STR(ev(n, "kwinrc", "Plugins", "blurEnabled"), "false");       /* el básico no desenfoca */
	CHECK_STR(ev(n, "kwinrc", "Plugins", "squashEnabled"), "true");
	CHECK_STR(ev(n, "kdeglobals", "KDE", "AnimationDurationFactor"), "0.35");
	from_text(&t, "[ui]\neffects = \"advanced\"\nblur = true\n");
	t.soft_render = 1;   /* máquina virtual sin 3D */
	n = edits(&t);
	CHECK_STR(ev(n, "kwinrc", "Plugins", "blurEnabled"), "false");
	CHECK_STR(ev(n, "kwinrc", "Plugins", "scaleEnabled"), "false");
	CHECK_STR(ev(n, "kdeglobals", "KDE", "AnimationDurationFactor"), "0");
	from_text(&t, "[ui]\nreduce_motion = true\n");
	n = edits(&t);
	CHECK_STR(ev(n, "kdeglobals", "KDE", "AnimationDurationFactor"), "0");
	CHECK_STR(ev(n, "kwinrc", "Plugins", "slideEnabled"), "false");

	/* ---- ventanas, escritorios, luz nocturna ---- */
	from_text(&t, "[ui]\nwindow_buttons = \"right\"\nworkspaces = 3\nnight_light = \"on\"\nnight_light_temp = 3500\n"
		      "hot_corners = false\n");
	n = edits(&t);
	CHECK_STR(ev(n, "kwinrc", "org.kde.kdecoration2", "ButtonsOnRight"), "IAX");
	CHECK_STR(ev(n, "kwinrc", "org.kde.kdecoration2", "ButtonsOnLeft"), "M");
	CHECK_STR(ev(n, "kwinrc", "Desktops", "Number"), "3");
	CHECK_STR(ev(n, "kwinrc", "NightColor", "Mode"), "Constant");
	CHECK_STR(ev(n, "kwinrc", "NightColor", "NightTemperature"), "3500");
	CHECK_STR(ev(n, "kwinrc", "Effect-overview", "BorderActivate"), "9");

	/* ---- fuentes, entrada, energía, bloqueo, sonido, indexador ---- */
	from_text(&t, "[ui]\nfont = \"Noto Sans\"\nfont_size = 12\n[input]\nkeymap = \"latam\"\nrepeat_delay_ms = 250\n"
		      "natural_scroll = false\npointer_speed = 50\nnumlock_on_boot = false\n"
		      "[power]\nscreen_off_minutes = 20\nsuspend_minutes = 0\nlid_action = \"ignore\"\n"
		      "[privacy]\nlock_minutes = 0\n[audio]\nmax_volume = 150\nevent_sounds = false\n"
		      "[services]\nindexer = true\n");
	n = edits(&t);
	CHECK_STR(ev(n, "kdeglobals", "General", "font"), "Noto Sans,12,-1,5,400,0,0,0,0,0,0,0,0,0,0,1");
	CHECK_STR(ev(n, "kxkbrc", "Layout", "LayoutList"), "latam");
	CHECK_STR(ev(n, "kcminputrc", "Keyboard", "RepeatDelay"), "250");
	CHECK_STR(ev(n, "kcminputrc", "Keyboard", "NumLock"), "1");
	CHECK_STR(ev(n, "kcminputrc", "Libinput][Defaults][Touchpad", "NaturalScroll"), "false");
	CHECK_STR(ev(n, "kcminputrc", "Libinput][Defaults][Pointer", "PointerAcceleration"), "0.50");
	CHECK_STR(ev(n, "powerdevilrc", "AC][Display", "TurnOffDisplayIdleTimeoutSec"), "1200");
	CHECK_STR(ev(n, "powerdevilrc", "Battery][Display", "TurnOffDisplayIdleTimeoutSec"), "600");   /* con batería, antes */
	CHECK_STR(ev(n, "powerdevilrc", "AC][SuspendAndShutdown", "AutoSuspendAction"), "0");
	CHECK_STR(ev(n, "powerdevilrc", "AC][SuspendAndShutdown", "LidAction"), "0");
	CHECK_STR(ev(n, "kscreenlockerrc", "Daemon", "Autolock"), "false");
	CHECK_STR(ev(n, "plasmaparc", "General", "RaiseMaximumVolume"), "true");
	CHECK_STR(ev(n, "kdeglobals", "Sounds", "Enable"), "false");
	CHECK_STR(ev(n, "baloofilerc", "Basic Settings", "Indexing-Enabled"), "true");
	CHECK_STR(ev(n, "ksmserverrc", "General", "loginMode"), "emptySession");

	/* ---- dock 2D / 3D ---- */
	from_text(&t, "[ui]\ndock_style = \"3d\"\ndock_icon_size = 56\ndock_position = \"left\"\ndock_hiding = \"auto\"\n");
	CHECK_STR(rros_desktoptheme(&t), "rros-3d");
	n = edits(&t);
	CHECK_STR(ev(n, "plasmarc", "Theme", "name"), "rros-3d");
	s = render(rros_render_plasma_layout, &t);
	CHECK(strstr(s, "dock.location = \"left\";") && strstr(s, "dock.height = 78;") &&
	      strstr(s, "dock.hiding = \"autohide\";"));
	CHECK(strstr(s, "org.kde.plasma.kickerdash") && strstr(s, "rros-launchpad") && strstr(s, "org.kde.plasma.trash"));
	CHECK(strstr(s, "bar.addWidget(\"org.rros.menu\")") && strstr(s, "org.kde.plasma.appmenu"));
	CHECK(strstr(s, "applications:rros-apps.desktop") && strstr(s, "applications:rros-settings.desktop"));
	free(s);
	/* clásico: barra de tareas abajo y sin dock */
	from_text(&t, "[ui]\npanel_position = \"bottom\"\npanel_size = 44\ndock = false\nclock_24h = false\n[panel]\ntaskbar = true\n");
	s = render(rros_render_plasma_layout, &t);
	CHECK(strstr(s, "bar.location = \"bottom\";") && strstr(s, "org.kde.plasma.taskmanager") &&
	      strstr(s, "org.kde.plasma.kickoff") && !strstr(s, "var dock") && strstr(s, "\"use24hFormat\", 0"));
	free(s);
	/* enfoque: barra en píldora con el reloj en el centro */
	from_text(&t, "[panel]\nstyle = \"pill\"\n");
	s = render(rros_render_plasma_layout, &t);
	CHECK(strstr(s, "bar.lengthMode = \"fit\";") && !strstr(s, "appmenu"));
	free(s);

	/* ---- terminal ---- */
	from_text(&t, "[term]\nscrollback = 20000\ncursor = \"beam\"\npadding = 4\n");
	s = render(rros_render_konsole_profile, &t);
	CHECK(strstr(s, "HistorySize=20000") && strstr(s, "CursorShape=1") && strstr(s, "TerminalMargin=4") &&
	      strstr(s, "ColorScheme=RROS"));
	free(s);

	/* ---- atajos y reglas del usuario ---- */
	from_text(&t, "[keybind]\nnav = \"Mod+Shift+b firefox\"\nmal = \"Mod+b\"\n[rule]\nmpv = \"app=mpv above\"\n"
		      "vid = \"title=Video opacity 0.8\"\n");
	n = edits(&t);
	CHECK_STR(ev(n, "kglobalshortcutsrc", "services][rros-kb-nav.desktop", "_launch"), "Meta+Shift+B");
	CHECK_STR(ev(n, "kglobalshortcutsrc", "services][rros-kb-mal.desktop", "_launch"), "");   /* inválido: fuera */
	CHECK_STR(ev(n, "kwinrulesrc", "rros-mpv", "wmclass"), "mpv");
	CHECK_STR(ev(n, "kwinrulesrc", "rros-mpv", "aboverule"), "2");
	CHECK_STR(ev(n, "kwinrulesrc", "rros-vid", "opacityactive"), "80");
	CHECK_STR(ev(n, "kwinrulesrc", "General", "rules"), "rros-mpv,rros-vid");

	/* ---- archivos INI de KDE: se mezclan sin perder lo del usuario ---- */
	{
		char dir[] = "/tmp/rros-ini-XXXXXX", path[300], val[64], *txt;
		FILE *f;

		CHECK(mkdtemp(dir) != NULL);
		snprintf(path, sizeof path, "%s/kwinrulesrc", dir);
		f = fopen(path, "w");
		fputs("[General]\ncount=2\nrules=mia,rros-viejo\n\n[mia]\nwmclass=gimp\n\n[rros-viejo]\nwmclass=x\n", f);
		fclose(f);
		CHECK(rros_ini_apply(path, "kwinrulesrc", ed, n) == 0);
		txt = slurp(path);
		CHECK(txt && strstr(txt, "rules=mia,rros-mpv,rros-vid") && strstr(txt, "count=3") && strstr(txt, "[mia]") &&
		      !strstr(txt, "rros-viejo") && strstr(txt, "[rros-mpv]\nDescription=RROS: mpv"));
		free(txt);
		/* las claves que ya están se cambian en su sitio; las demás, al final de su grupo */
		snprintf(path, sizeof path, "%s/kwinrc", dir);
		f = fopen(path, "w");
		fputs("[Plugins]\nblurEnabled=true\nmio=1\n\n[Otro]\na=b\n", f);
		fclose(f);
		from_text(&t, "");
		n = edits(&t);
		CHECK(rros_ini_apply(path, "kwinrc", ed, n) == 0);
		CHECK(rros_ini_get(path, "Plugins", "blurEnabled", val, sizeof val) == 0);
		CHECK_STR(val, "false");
		CHECK(rros_ini_get(path, "Plugins", "mio", val, sizeof val) == 0);
		CHECK_STR(val, "1");
		CHECK(rros_ini_get(path, "Otro", "a", val, sizeof val) == 0);
		CHECK(rros_ini_get(path, "Desktops", "Number", val, sizeof val) == 0);
		CHECK_STR(val, "1");
		CHECK(rros_ini_get(path, "Nada", "x", val, sizeof val) == -1 && val[0] == '\0');
		/* dos veces da lo mismo */
		txt = slurp(path);
		CHECK(rros_ini_apply(path, "kwinrc", ed, n) == 0);
		{
			char *again = slurp(path);

			CHECK(txt && again && !strcmp(txt, again));
			free(again);
		}
		free(txt);
		/* lanzadores de atajos y de inicio */
		from_text(&t, "[keybind]\nnav = \"Mod+b firefox --private-window\"\n[autostart]\nsync = \"syncthing\"\n");
		snprintf(path, sizeof path, "%s/apps", dir);
		{
			char autos[220];

			snprintf(autos, sizeof autos, "%s/auto", dir);
			CHECK(rros_write_launchers(&cur, path, autos) == 0);
			snprintf(path, sizeof path, "%s/apps/rros-kb-nav.desktop", dir);
			txt = slurp(path);
			CHECK(txt && strstr(txt, "Exec=firefox --private-window\n") && strstr(txt, "NoDisplay=true"));
			free(txt);
			snprintf(path, sizeof path, "%s/rros-auto-sync.desktop", autos);
			CHECK(access(path, R_OK) == 0);
		}
		snprintf(path, sizeof path, "rm -rf '%s'", dir);
		CHECK(system(path) == 0);
	}

	/* ---- estilos de escritorio ---- */
	{
		const struct rros_desktop_preset *d = rros_desktop_find("clasico");

		CHECK(d != NULL && rros_desktop_find("mosaico") == NULL && rros_desktop_presets_len == 3);
		CHECK(rros_desktop_find("moderno") && rros_desktop_find("enfoque"));
		/* todas sus claves pasan la validación del esquema */
		for (size_t i = 0; i < rros_desktop_presets_len; i++)
			for (size_t k = 0; k < 14 && rros_desktop_presets[i].keys[k][0]; k++) {
				const struct rros_key *key = rros_schema_find(rros_desktop_presets[i].keys[k][0]);
				char err[200];

				CHECK(key != NULL);
				if (key)
					CHECK(rros_schema_check(key, rros_desktop_presets[i].keys[k][1],
								key->type == RROS_T_STR || key->type == RROS_T_ENUM, err, sizeof err) == 0);
			}
		from_text(&t, "[ui]\npanel_position = \"bottom\"\npanel_size = 44\ndock = false\nglobal_menu = false\n"
			      "panel_autohide = false\n[panel]\nstyle = \"flat\"\ntaskbar = true\n");
		CHECK(rros_desktop_current(&cur) == d);
		from_text(&t, "[ui]\npanel_position = \"top\"\n");
		CHECK(rros_desktop_current(&cur) == NULL);
	}

	/* ---- KDE Plasma: colores y barras de título con los colores del tema ---- */
	{
		struct rros_palette pal;
		char dir[] = "/tmp/rros-aurorae-XXXXXX", path[200], *svg;

		from_text(&t, "");
		s = render(rros_render_kde_colors, &t);
		CHECK(strstr(s, "[Colors:Window]\nBackgroundNormal=34,13,19\n") != NULL);      /* superficie de Lirio */
		CHECK(strstr(s, "[Colors:Selection]\nBackgroundNormal=200,36,60\n") != NULL);  /* el acento */
		CHECK(strstr(s, "[WM]\nactiveBackground=34,13,19\n") != NULL);
		free(s);
		rros_theme_palette(&t, &pal);
		CHECK(mkdtemp(dir) != NULL);
		snprintf(path, sizeof path, "%s/RROS", dir);
		CHECK(rros_write_aurorae(path, &t, &pal) == 0);
		snprintf(path, sizeof path, "%s/RROS/decoration.svg", dir);
		svg = slurp(path);
		CHECK(svg && strstr(svg, "id=\"decoration-topleft\"") && strstr(svg, "id=\"decoration-inactive-bottomright\"") &&
		      strstr(svg, "fill=\"#220d13\""));
		free(svg);
		snprintf(path, sizeof path, "%s/RROS/close.svg", dir);
		CHECK(access(path, R_OK) == 0);
		snprintf(path, sizeof path, "rm -rf '%s'", dir);
		CHECK(system(path) == 0);
	}
	rros_conf_free(&cur);
	DONE();
}
