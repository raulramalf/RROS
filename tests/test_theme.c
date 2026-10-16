#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "theme.h"

/* Renderiza a una cadena (open_memstream). El llamador libera. */
static char *render(rros_render_fn fn, const struct rros_theme *t)
{
	struct rros_palette p;
	char *buf = NULL;
	size_t len = 0;
	FILE *f = open_memstream(&buf, &len);

	rros_theme_palette(t, &p);
	fn(t, &p, f);
	fclose(f);
	return buf;
}

static void from_text(struct rros_theme *t, const char *text)
{
	struct rros_conf c;

	rros_conf_init(&c);
	CHECK(rros_conf_parse(&c, text, NULL) == 0);
	rros_theme_from_conf(t, &c);
	rros_conf_free(&c);
}

int main(void)
{
	struct rros_theme t;
	char *s;

	/* valores por defecto */
	from_text(&t, "");
	CHECK(t.dark == 1);
	CHECK_STR(t.accent, "#7c5cff");
	CHECK(t.gap == 8 && t.border == 2);
	CHECK_STR(t.panel_pos, "top");

	/* los ajustes llegan al sway generado */
	from_text(&t, "[ui]\naccent = \"#FF8800\"\ngap = 14\nborder_width = 4\nscale = 150\n"
		      "layout = \"floating\"\nworkspaces = 3\n[input]\nkeymap = \"latam\"\npointer_speed = -50\n");
	s = render(rros_render_sway, &t);
	CHECK(strstr(s, "gaps inner 14") != NULL);
	CHECK(strstr(s, "gaps outer 7") != NULL);
	CHECK(strstr(s, "default_border pixel 4") != NULL);
	CHECK(strstr(s, "#ff8800 #ff8800 #e6ecff") != NULL);
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
	CHECK(strstr(s, "#7c5cff") != NULL);
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
	DONE();
}
