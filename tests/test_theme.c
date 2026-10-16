#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "theme.h"

static struct rros_conf cur;

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

int main(void)
{
	struct rros_theme t;
	char *s;

	/* valores por defecto */
	from_text(&t, "");
	CHECK(t.dark == 1);
	CHECK_STR(t.accent, "");
	CHECK_STR(t.theme, "aurora");
	CHECK(t.gap == 8 && t.border == 2);
	CHECK_STR(t.panel_pos, "top");

	/* los ajustes llegan al sway generado */
	from_text(&t, "[ui]\naccent = \"#FF8800\"\ngap = 14\nborder_width = 4\nscale = 150\n"
		      "layout = \"floating\"\nworkspaces = 3\n[input]\nkeymap = \"latam\"\npointer_speed = -50\n");
	s = render(rros_render_sway, &t);
	CHECK(strstr(s, "gaps inner 14") != NULL);
	CHECK(strstr(s, "gaps outer 7") != NULL);
	CHECK(strstr(s, "default_border pixel 4") != NULL);
	CHECK(strstr(s, "#ff8800 #ff8800 #0b1020") != NULL);
	CHECK(strstr(s, "output * scale 1.50") != NULL);
	CHECK(strstr(s, "xkb_layout latam") != NULL);
	CHECK(strstr(s, "pointer_accel -0.50") != NULL);
	CHECK(strstr(s, "floating enable") != NULL);
	CHECK(strstr(s, "workspace number 3") != NULL);
	CHECK(strstr(s, "workspace number 4") == NULL);
	CHECK(strstr(s, RROS_GENERATED) != NULL);
	free(s);

	/* valores peligrosos se descartan: no se puede inyectar comandos en sway */
	from_text(&t, "[ui]\naccent = \"red; exec evil\"\nfont = \"Inter\\nexec evil\"\n[input]\nkeymap = \"es\\nexec x\"\n");
	s = render(rros_render_sway, &t);
	CHECK(strstr(s, "evil") == NULL);
	CHECK(strstr(s, "exec x") == NULL);
	CHECK(strstr(s, "#7c5cff") != NULL); /* acento del tema Aurora */
	CHECK(strstr(s, "font pango:Inter 11") != NULL);
	free(s);

	/* valores fuera de rango se acotan */
	from_text(&t, "[ui]\ngap = 99999\nfont_size = 1\n");
	CHECK(t.gap == 64);
	CHECK(t.font_size == 6);

	/* esquema claro */
	from_text(&t, "[ui]\ncolor_scheme = \"light\"\n");
	s = render(rros_render_foot, &t);
	CHECK(strstr(s, "background=f5f7ff") != NULL);
	free(s);

	/* waybar: posición, reloj y transparencia */
	from_text(&t, "[ui]\npanel_position = \"left\"\npanel_size = 48\nclock_24h = false\ntransparency = 40\n");
	s = render(rros_render_waybar, &t);
	CHECK(strstr(s, "\"position\": \"left\"") != NULL);
	CHECK(strstr(s, "\"width\": 48") != NULL);
	CHECK(strstr(s, "%I:%M %p") != NULL);
	free(s);
	s = render(rros_render_waybar_css, &t);
	CHECK(strstr(s, "rgba(11, 16, 32, 0.60)") != NULL);
	CHECK(strstr(s, "border-right: 2px solid") != NULL);
	free(s);

	/* efectos: solo con compositor full y no en off */
	from_text(&t, "[ui]\ncompositor = \"minimal\"\nblur = true\neffects = \"advanced\"\n");
	s = render(rros_render_effects, &t);
	CHECK(strstr(s, "blur") == NULL);
	free(s);
	from_text(&t, "[ui]\ncompositor = \"full\"\nblur = true\nshadows = true\neffects = \"advanced\"\ncorner_radius = 12\n");
	s = render(rros_render_effects, &t);
	CHECK(strstr(s, "corner_radius 12") != NULL);
	CHECK(strstr(s, "blur enable") != NULL);
	CHECK(strstr(s, "blur_passes 3") != NULL);
	CHECK(strstr(s, "shadows enable") != NULL);
	free(s);
	from_text(&t, "[ui]\ncompositor = \"full\"\neffects = \"off\"\nblur = true\n");
	s = render(rros_render_effects, &t);
	CHECK(strstr(s, "blur") == NULL);
	free(s);

	/* ---- temas incluidos ---- */
	from_text(&t, "[ui]\ntheme = \"nord\"\n");
	s = render(rros_render_foot, &t);
	CHECK(strstr(s, "background=2e3440") != NULL);
	CHECK(strstr(s, "selection-background=88c0d0") != NULL); /* acento propio de Nord */
	free(s);
	from_text(&t, "[ui]\ntheme = \"nord\"\naccent = \"#ff0000\"\n");
	s = render(rros_render_foot, &t);
	CHECK(strstr(s, "selection-background=ff0000") != NULL); /* el usuario manda */
	free(s);
	from_text(&t, "[ui]\ntheme = \"gruvbox\"\ncolor_scheme = \"light\"\n");
	s = render(rros_render_foot, &t);
	CHECK(strstr(s, "background=fbf1c7") != NULL);
	free(s);
	{
		struct rros_palette p;

		from_text(&t, "[ui]\ntheme = \"no-existe\"\n");
		CHECK(rros_theme_palette(&t, &p) == 1); /* cae a aurora */
		CHECK_STR(p.bg, "#0b1020");
		from_text(&t, "[ui]\ntheme = \"../../etc/passwd\"\n");
		CHECK_STR(t.theme, "aurora"); /* nombre inválido: nunca se usa como ruta */
	}
	{
		struct rros_theme_info list[64];
		size_t n = rros_theme_list(list, 64);

		CHECK(n >= 7);
		CHECK(list[0].builtin == 1);
	}

	/* ---- panel configurable ---- */
	from_text(&t, "[panel]\nstyle = \"floating\"\nleft = \"workspaces,window\"\nright = \"cpu,hackme,battery\"\n");
	s = render(rros_render_waybar, &t);
	CHECK(strstr(s, "\"modules-left\": [\"sway/workspaces\", \"sway/window\"]") != NULL);
	CHECK(strstr(s, "\"modules-right\": [\"cpu\", \"battery\"]") != NULL); /* módulo desconocido, descartado */
	CHECK(strstr(s, "hackme") == NULL);
	CHECK(strstr(s, "\"margin\": \"6 8 0 8\"") != NULL);
	free(s);
	s = render(rros_render_waybar_css, &t);
	CHECK(strstr(s, ".modules-left, .modules-center, .modules-right") != NULL);
	free(s);
	from_text(&t, "[panel]\nstyle = \"pill\"\n");
	s = render(rros_render_waybar_css, &t);
	CHECK(strstr(s, "border-radius: 999px") != NULL);
	free(s);

	/* ---- atajos, reglas y programas de inicio del usuario ---- */
	from_text(&t, "[keybind]\nnav = \"Mod+Shift+b exec firefox\"\nmal = \"Mod+x rm -rf\"\n"
		      "[rule]\nff = \"app_id=firefox workspace 2\"\nfloat = \"title=Calculadora floating\"\n"
		      "[autostart]\nsync = \"syncthing -no-browser\"\n");
	s = render(rros_render_sway, &t);
	CHECK(strstr(s, "bindsym $mod+Shift+b exec firefox") != NULL);
	CHECK(strstr(s, "# ignorado (no válido): keybind.mal") != NULL);
	CHECK(strstr(s, "assign [app_id=\"firefox\"] workspace number 2") != NULL);
	CHECK(strstr(s, "for_window [title=\"Calculadora\"] floating enable") != NULL);
	CHECK(strstr(s, "exec syncthing -no-browser") != NULL);
	CHECK(strstr(s, "rm -rf") == NULL);
	free(s);

	/* ---- GTK, mako, fuzzel ---- */
	from_text(&t, "[ui]\ntheme = \"dracula\"\ncolor_scheme = \"dark\"\nfont_size = 13\n");
	s = render(rros_render_gtk_css, &t);
	CHECK(strstr(s, "@define-color window_bg_color #282a36;") != NULL);
	CHECK(strstr(s, "@define-color accent_bg_color #bd93f9;") != NULL);
	CHECK(strstr(s, "@define-color theme_bg_color #282a36;") != NULL);
	free(s);
	s = render(rros_render_gtk_settings, &t);
	CHECK(strstr(s, "gtk-application-prefer-dark-theme=1") != NULL);
	CHECK(strstr(s, "gtk-font-name=Inter 13") != NULL);
	free(s);
	s = render(rros_render_mako, &t);
	CHECK(strstr(s, "background-color=#282a36ff") != NULL);
	CHECK(strstr(s, "border-color=#bd93f9") != NULL);
	free(s);
	s = render(rros_render_fuzzel, &t);
	CHECK(strstr(s, "background=282a36ff") != NULL);
	free(s);

	/* ---- terminal ---- */
	from_text(&t, "[term]\nscrollback = 20000\ncursor = \"beam\"\npadding = 20\n");
	s = render(rros_render_foot, &t);
	CHECK(strstr(s, "lines=20000") != NULL);
	CHECK(strstr(s, "style=beam") != NULL);
	CHECK(strstr(s, "pad=20x16") != NULL);
	free(s);

	rros_conf_free(&cur);
	DONE();
}
