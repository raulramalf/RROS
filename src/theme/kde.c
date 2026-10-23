/* KDE Plasma: esquema de colores (.colors) desde la paleta de Lily. Lo aplica plasma-apply-colorscheme, y así
 * el tema llega a las ventanas, los paneles, el dock, las barras de título y las aplicaciones Qt y KDE. */
#include "theme.h"
#include "rros_profile.h"

#include <stdio.h>

/* mezcla a con b: t = 0 da a, t = 1 da b */
static void mix(const char *a, const char *b, double t, char out[12])
{
	int r1, g1, b1, r2, g2, b2;

	rros_hex_rgb(a, &r1, &g1, &b1);
	rros_hex_rgb(b, &r2, &g2, &b2);
	snprintf(out, 12, "%d,%d,%d", (int)(r1 + (r2 - r1) * t + .5), (int)(g1 + (g2 - g1) * t + .5),
		 (int)(b1 + (b2 - b1) * t + .5));
}

static void rgb(const char *hex, char out[12])
{
	mix(hex, hex, 0, out);
}

/* texto legible sobre el color de acento: el fondo del tema o blanco, el que más contraste */
static const char *on_accent(const struct rros_palette *p)
{
	int r, g, b;

	rros_hex_rgb(p->accent, &r, &g, &b);
	return 0.299 * r + 0.587 * g + 0.114 * b > 150 ? "#101010" : "#ffffff";
}

static void group(FILE *f, const char *name, const struct rros_palette *p, const char *bg)
{
	char c[12];

	fprintf(f, "[Colors:%s]\n", name);
	rgb(bg, c);
	fprintf(f, "BackgroundNormal=%s\n", c);
	mix(bg, p->text, 0.04, c);
	fprintf(f, "BackgroundAlternate=%s\n", c);
	rgb(p->accent, c);
	fprintf(f, "DecorationFocus=%s\nDecorationHover=%s\n", c, c);
	rgb(p->text, c);
	fprintf(f, "ForegroundNormal=%s\n", c);
	rgb(p->muted, c);
	fprintf(f, "ForegroundInactive=%s\n", c);
	rgb(p->accent, c);
	fprintf(f, "ForegroundActive=%s\n", c);
	rgb(p->blue, c);
	fprintf(f, "ForegroundLink=%s\n", c);
	rgb(p->magenta, c);
	fprintf(f, "ForegroundVisited=%s\n", c);
	rgb(p->red, c);
	fprintf(f, "ForegroundNegative=%s\n", c);
	rgb(p->yellow, c);
	fprintf(f, "ForegroundNeutral=%s\n", c);
	rgb(p->green, c);
	fprintf(f, "ForegroundPositive=%s\n\n", c);
}

void rros_render_kde_colors(const struct rros_theme *t, const struct rros_palette *p, const struct rros_conf *c, FILE *f)
{
	char raised[8], col[12], col2[12];
	int r, g, b;

	(void)c;
	/* botones y barras: un poco más claros (u oscuros) que la superficie */
	mix(p->surface, p->text, t->dark ? 0.08 : 0.05, col);
	sscanf(col, "%d,%d,%d", &r, &g, &b);
	snprintf(raised, sizeof raised, "#%02x%02x%02x", r, g, b);

	fprintf(f, "# %s. No lo edites: cambia el tema con «rros theme» o en Lily Ajustes.\n", RROS_GENERATED);
	fprintf(f, "[General]\nColorScheme=Lily\nName=Lily (%s)\nshadeSortColumn=true\n\n", t->theme);
	fprintf(f, "[KDE]\ncontrast=4\n\n");
	fprintf(f, "[ColorEffects:Disabled]\nColor=56,56,56\nColorAmount=0\nColorEffect=0\nContrastAmount=0.65\n"
		   "ContrastEffect=1\nIntensityAmount=0.1\nIntensityEffect=2\n\n");
	fprintf(f, "[ColorEffects:Inactive]\nChangeSelectionColor=true\nColor=112,111,110\nColorAmount=0.025\n"
		   "ColorEffect=2\nContrastAmount=0.1\nContrastEffect=2\nEnable=false\nIntensityAmount=0\nIntensityEffect=0\n\n");
	group(f, "View", p, p->bg);
	group(f, "Window", p, p->surface);
	group(f, "Button", p, raised);
	group(f, "Header", p, p->surface);
	fputs("[Colors:Header][Inactive]\n", f);
	rgb(p->bg, col);
	fprintf(f, "BackgroundNormal=%s\n", col);
	rgb(p->muted, col);
	fprintf(f, "ForegroundNormal=%s\n\n", col);
	group(f, "Tooltip", p, p->surface);
	group(f, "Complementary", p, p->bg);

	/* selección: el acento, con su texto legible */
	fputs("[Colors:Selection]\n", f);
	rgb(p->accent, col);
	mix(p->accent, p->bg, 0.25, col2);
	fprintf(f, "BackgroundNormal=%s\nBackgroundAlternate=%s\nDecorationFocus=%s\nDecorationHover=%s\n", col, col2, col, col);
	rgb(on_accent(p), col);
	fprintf(f, "ForegroundNormal=%s\nForegroundActive=%s\n", col, col);
	mix(on_accent(p), p->accent, 0.3, col2);
	fprintf(f, "ForegroundInactive=%s\n", col2);
	rgb(p->blue, col);
	fprintf(f, "ForegroundLink=%s\nForegroundVisited=%s\n", col, col);
	rgb(p->red, col);
	fprintf(f, "ForegroundNegative=%s\n", col);
	rgb(p->yellow, col);
	fprintf(f, "ForegroundNeutral=%s\n", col);
	rgb(p->green, col);
	fprintf(f, "ForegroundPositive=%s\n\n", col);

	/* barras de título de las ventanas */
	fputs("[WM]\n", f);
	rgb(p->surface, col);
	fprintf(f, "activeBackground=%s\n", col);
	rgb(p->text, col);
	fprintf(f, "activeForeground=%s\n", col);
	rgb(p->accent, col);
	fprintf(f, "activeBlend=%s\n", col);
	rgb(p->bg, col);
	fprintf(f, "inactiveBackground=%s\n", col);
	rgb(p->muted, col);
	fprintf(f, "inactiveForeground=%s\ninactiveBlend=%s\n", col, col);
}

/* ---- decoración de ventanas (tema Aurorae de KWin): barra de título del color del tema y los tres botones de
 *      «semáforo» a la izquierda (cerrar, minimizar, maximizar), como en macOS. Las piezas siguen el formato
 *      FrameSvg de Aurorae: decoration-{topleft,top,...} y decoration-inactive-* en decoration.svg, y
 *      {active,hover,pressed,inactive,deactivated}-center en cada botón. ---- */

static int write_file(const char *dir, const char *name, const char *text)
{
	char path[900];
	FILE *f;

	snprintf(path, sizeof path, "%s/%s", dir, name);
	if (!(f = fopen(path, "w")))
		return -1;
	fputs(text, f);
	return fclose(f);
}

/* marco de 9 piezas de 12x12; las esquinas de arriba, redondeadas */
static void frame(FILE *f, const char *prefix, int x0, const char *fill, const char *line)
{
	static const char *const part[3][3] = { { "topleft", "top", "topright" }, { "left", "center", "right" },
						{ "bottomleft", "bottom", "bottomright" } };

	for (int r = 0; r < 3; r++)
		for (int c = 0; c < 3; c++) {
			int x = x0 + c * 12, y = r * 12;

			if (r == 0 && c == 0)
				fprintf(f, "<path id=\"%s-%s\" d=\"M%d,%d v-2 a10,10 0 0 1 10,-10 h2 v12 z\" fill=\"%s\"/>\n",
					prefix, part[r][c], x, y + 12, fill);
			else if (r == 0 && c == 2)
				fprintf(f, "<path id=\"%s-%s\" d=\"M%d,%d h2 a10,10 0 0 1 10,10 v2 h-12 z\" fill=\"%s\"/>\n",
					prefix, part[r][c], x, y, fill);
			else
				fprintf(f, "<rect id=\"%s-%s\" x=\"%d\" y=\"%d\" width=\"12\" height=\"12\" fill=\"%s\"/>\n",
					prefix, part[r][c], x, y, fill);
		}
	/* línea fina bajo la barra de título no: la separación la da el contenido de la ventana */
	(void)line;
}

/* botón de la barra de título: el símbolo siempre visible, en el color del texto, sobre un círculo suave que se
 * marca al pasar el ratón; el de cerrar se pone rojo con el símbolo blanco. Cada estado ocupa una celda de 28x24. */
static int button(const char *dir, const char *name, int danger, const char *glyph, const struct rros_palette *p)
{
	char path[900];
	FILE *f;
	static const char *const state[] = { "active", "hover", "pressed", "inactive", "deactivated" };

	snprintf(path, sizeof path, "%s/%s.svg", dir, name);
	if (!(f = fopen(path, "w")))
		return -1;
	fputs("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"140\" height=\"24\" viewBox=\"0 0 140 24\">\n", f);
	for (int i = 0; i < 5; i++) {
		int x = i * 28;
		const char *bg = "none", *fg = i >= 3 ? p->muted : p->text;
		double op = 0;

		if (i == 1 || i == 2) {
			bg = danger ? "#e5484d" : p->text;
			op = danger ? (i == 2 ? 0.8 : 1) : (i == 2 ? 0.22 : 0.13);
			if (danger)
				fg = "#ffffff";
		}
		fprintf(f, "<g id=\"%s-center\"><rect x=\"%d\" y=\"0\" width=\"28\" height=\"24\" fill=\"none\"/>", state[i], x);
		if (op > 0)
			fprintf(f, "<circle cx=\"%d\" cy=\"12\" r=\"11\" fill=\"%s\" fill-opacity=\"%.2f\"/>", x + 14, bg, op);
		fprintf(f, "<path transform=\"translate(%d,0)\" d=\"%s\" stroke=\"%s\" stroke-opacity=\"%s\" stroke-width=\"1.5\" "
			   "stroke-linecap=\"round\" stroke-linejoin=\"round\" fill=\"none\"/></g>\n",
			x, glyph, fg, i == 4 ? "0.45" : "1");
	}
	fputs("</svg>\n", f);
	return fclose(f);
}

int rros_write_aurorae(const char *dir, const struct rros_theme *t, const struct rros_palette *p)
{
	char path[900], text[1400];
	int r, g, b, r2, g2, b2;
	FILE *f;

	(void)t;
	rros_hex_rgb(p->text, &r, &g, &b);
	rros_hex_rgb(p->muted, &r2, &g2, &b2);
	snprintf(text, sizeof text,
		 "# %s\n[General]\nActiveTextColor=%d,%d,%d\nInactiveTextColor=%d,%d,%d\nTitleAlignment=Center\n"
		 "TitleVerticalAlignment=Center\nAnimation=120\nUseTextShadow=false\nShadow=true\nHaloActive=false\n"
		 "HaloInactive=false\n\n[Layout]\nBorderLeft=0\nBorderRight=0\nBorderBottom=0\nTitleEdgeTop=6\n"
		 "TitleEdgeBottom=6\nTitleEdgeLeft=8\nTitleEdgeRight=8\nTitleBorderLeft=10\nTitleBorderRight=10\n"
		 "TitleHeight=24\nButtonWidth=28\nButtonHeight=24\nButtonSpacing=4\nButtonMarginTop=0\n"
		 "ExplicitButtonSpacer=10\nPaddingTop=0\nPaddingBottom=0\nPaddingLeft=0\nPaddingRight=0\n",
		 RROS_GENERATED, r, g, b, r2, g2, b2);
	snprintf(path, sizeof path, "%s/Lilyrc", dir);
	if (rros_mkdir_parents(path) || write_file(dir, "Lilyrc", text))
		return -1;
	if (write_file(dir, "metadata.desktop",
		       "[Desktop Entry]\nName=Lily\nComment=Barras de título de Lily\n"
		       "X-KDE-PluginInfo-Author=Lily\nX-KDE-PluginInfo-Name=Lily\nX-KDE-PluginInfo-Version=1.0\n"
		       "X-KDE-PluginInfo-License=GPL\nX-KDE-PluginInfo-EnabledByDefault=true\n"))
		return -1;
	snprintf(path, sizeof path, "%s/decoration.svg", dir);
	if (!(f = fopen(path, "w")))
		return -1;
	fputs("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"72\" height=\"36\" viewBox=\"0 0 72 36\">\n", f);
	frame(f, "decoration", 0, p->surface, p->border);
	frame(f, "decoration-inactive", 36, p->bg, p->border);
	fputs("</svg>\n", f);
	if (fclose(f))
		return -1;
	/* cerrar (rojo al pasar el ratón), minimizar, maximizar y restaurar (dos ventanas superpuestas) */
	if (button(dir, "close", 1, "M9.5,7.5 L18.5,16.5 M18.5,7.5 L9.5,16.5", p) ||
	    button(dir, "minimize", 0, "M9,12.5 H19", p) ||
	    button(dir, "maximize", 0, "M10.5,7.5 H17.5 Q19.5,7.5 19.5,9.5 V14.5 Q19.5,16.5 17.5,16.5 H10.5 Q8.5,16.5 8.5,14.5 V9.5 Q8.5,7.5 10.5,7.5 Z", p) ||
	    button(dir, "restore", 0, "M8.5,10 H16 V17 H8.5 Z M11,10 V7.5 H19 V14.5 H16", p))
		return -1;
	return 0;
}
