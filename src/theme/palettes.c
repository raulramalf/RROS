/* Paletas incluidas. Cada tema tiene variante oscura y clara. */
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "rros_profile.h"
#include "theme.h"

struct pal { const char *bg, *surface, *text, *muted, *border, *red, *green, *yellow, *blue, *magenta, *cyan, *accent; };
struct def { const char *name; struct pal dark, light; };

static const struct def builtin[] = {
	/* lirio: los colores del logotipo (vino, rojo lirio y champán) */
	{ "lirio",
	  { "#140609", "#220d13", "#f6ecdf", "#b49c92", "#3d1a22", "#ff5a6e", "#7cc79a", "#e9c48a", "#8fb3e0", "#e0567a", "#efc9a6", "#c8243c" },
	  { "#faf5ef", "#ffffff", "#2a0f16", "#7d6460", "#ead8cf", "#c0283e", "#2f7d4f", "#9a6b1f", "#3b6ea8", "#a8325a", "#a8754a", "#a51d33" } },
	{ "aurora",
	  { "#0b1020", "#141c38", "#e6ecff", "#8a96bd", "#26305a", "#ff6b81", "#4ade80", "#fbbf24", "#60a5fa", "#c084fc", "#22d3ee", "#7c5cff" },
	  { "#f5f7ff", "#ffffff", "#141a33", "#5b6489", "#d5dbf5", "#d6334f", "#16a34a", "#b7791f", "#2563eb", "#9333ea", "#0891b2", "#5b3df5" } },
	{ "nord",
	  { "#2e3440", "#3b4252", "#eceff4", "#8f9bb3", "#4c566a", "#bf616a", "#a3be8c", "#ebcb8b", "#81a1c1", "#b48ead", "#88c0d0", "#88c0d0" },
	  { "#eceff4", "#e5e9f0", "#2e3440", "#4c566a", "#d8dee9", "#bf616a", "#6f8f55", "#a67c00", "#5e81ac", "#8f6a9a", "#4a8da3", "#5e81ac" } },
	{ "dracula",
	  { "#282a36", "#343746", "#f8f8f2", "#8a93c5", "#44475a", "#ff5555", "#50fa7b", "#f1fa8c", "#8be9fd", "#ff79c6", "#8be9fd", "#bd93f9" },
	  { "#fffbeb", "#f5f0dc", "#1f1f1f", "#6c664b", "#e0d9bd", "#cb3a2a", "#14710a", "#846e15", "#036a96", "#a3144d", "#036a96", "#644ac9" } },
	{ "gruvbox",
	  { "#282828", "#3c3836", "#ebdbb2", "#928374", "#504945", "#fb4934", "#b8bb26", "#fabd2f", "#83a598", "#d3869b", "#8ec07c", "#fabd2f" },
	  { "#fbf1c7", "#ebdbb2", "#3c3836", "#7c6f64", "#d5c4a1", "#cc241d", "#98971a", "#d79921", "#458588", "#b16286", "#689d6a", "#d79921" } },
	{ "solarized",
	  { "#002b36", "#073642", "#93a1a1", "#657b83", "#586e75", "#dc322f", "#859900", "#b58900", "#268bd2", "#d33682", "#2aa198", "#268bd2" },
	  { "#fdf6e3", "#eee8d5", "#586e75", "#93a1a1", "#d9d2bd", "#dc322f", "#859900", "#b58900", "#268bd2", "#d33682", "#2aa198", "#268bd2" } },
	{ "mono",
	  { "#111111", "#1a1a1a", "#eeeeee", "#8a8a8a", "#333333", "#e06c75", "#98c379", "#e5c07b", "#61afef", "#c678dd", "#56b6c2", "#ffffff" },
	  { "#fafafa", "#ffffff", "#111111", "#6b6b6b", "#dddddd", "#c0392b", "#2e7d32", "#9a6700", "#1565c0", "#7b1fa2", "#00796b", "#111111" } },
	{ "contrast",
	  { "#000000", "#0a0a0a", "#ffffff", "#d0d0d0", "#ffffff", "#ff6666", "#66ff66", "#ffff66", "#66b3ff", "#ff66ff", "#66ffff", "#ffff00" },
	  { "#ffffff", "#f2f2f2", "#000000", "#333333", "#000000", "#b00020", "#006400", "#7a5200", "#0033cc", "#800080", "#006666", "#0000cc" } },
};
#define NBUILTIN (sizeof builtin / sizeof builtin[0])

static void put(char dst[8], const char *hex)
{
	snprintf(dst, 8, "%s", hex);
}

static void from_pal(struct rros_palette *p, const struct pal *s)
{
	put(p->bg, s->bg); put(p->surface, s->surface); put(p->text, s->text); put(p->muted, s->muted);
	put(p->border, s->border); put(p->red, s->red); put(p->green, s->green); put(p->yellow, s->yellow);
	put(p->blue, s->blue); put(p->magenta, s->magenta); put(p->cyan, s->cyan); put(p->accent, s->accent);
}

static const struct def *find_builtin(const char *name)
{
	for (size_t i = 0; i < NBUILTIN; i++)
		if (!strcmp(builtin[i].name, name))
			return &builtin[i];
	return NULL;
}

static int valid_hex(const char *s)
{
	if (strlen(s) != 7 || s[0] != '#')
		return 0;
	for (int i = 1; i < 7; i++)
		if (!isxdigit((unsigned char)s[i]))
			return 0;
	return 1;
}

int rros_themes_dir(char *buf, size_t n)
{
	const char *env = getenv("RROS_THEMES_DIR"), *x = getenv("XDG_CONFIG_HOME"), *h = getenv("HOME");
	int len;

	if (env && *env)
		len = snprintf(buf, n, "%s", env);
	else if (x && *x)
		len = snprintf(buf, n, "%s/rros/themes", x);
	else if (h && *h)
		len = snprintf(buf, n, "%s/.config/rros/themes", h);
	else
		return -1;
	return (len < 0 || (size_t)len >= n) ? -1 : 0;
}

static int theme_file(const char *name, char *buf, size_t n)
{
	char dir[900];

	if (!rros_valid_profile_name(name) || rros_themes_dir(dir, sizeof dir))
		return -1;
	snprintf(buf, n, "%s/%s.toml", dir, name);
	if (access(buf, R_OK) == 0)
		return 0;
	snprintf(buf, n, "/usr/share/rros/themes/%s.toml", name);
	return access(buf, R_OK) == 0 ? 0 : -1;
}

static void apply_color(struct rros_palette *p, const char *k, const char *v)
{
	if (!valid_hex(v))
		return;
	if (!strcmp(k, "bg")) put(p->bg, v);
	else if (!strcmp(k, "surface")) put(p->surface, v);
	else if (!strcmp(k, "text")) put(p->text, v);
	else if (!strcmp(k, "muted")) put(p->muted, v);
	else if (!strcmp(k, "border")) put(p->border, v);
	else if (!strcmp(k, "red")) put(p->red, v);
	else if (!strcmp(k, "green")) put(p->green, v);
	else if (!strcmp(k, "yellow")) put(p->yellow, v);
	else if (!strcmp(k, "blue")) put(p->blue, v);
	else if (!strcmp(k, "magenta")) put(p->magenta, v);
	else if (!strcmp(k, "cyan")) put(p->cyan, v);
	else if (!strcmp(k, "accent")) put(p->accent, v);
}

/* depth limita la herencia (base de base de base...). Devuelve 0 si lo encontró. */
static int load_palette(const char *name, int dark, struct rros_palette *p, int depth)
{
	const struct def *d = find_builtin(name);
	struct rros_conf c;
	char path[1100];
	const char *base;
	static const char *keys[] = { "bg", "surface", "text", "muted", "border", "red", "green", "yellow",
				      "blue", "magenta", "cyan", "accent" };

	if (d) {
		from_pal(p, dark ? &d->dark : &d->light);
		return 0;
	}
	if (depth > 4 || theme_file(name, path, sizeof path))
		return 1;
	rros_conf_init(&c);
	if (rros_conf_load(&c, path, NULL) != 0) {
		rros_conf_free(&c);
		return 1;
	}
	base = rros_conf_get(&c, "theme.base");
	if (!base || !rros_valid_profile_name(base) || load_palette(base, dark, p, depth + 1))
		from_pal(p, dark ? &builtin[0].dark : &builtin[0].light);
	for (size_t i = 0; i < sizeof keys / sizeof *keys; i++) {
		char key[64];
		const char *v;

		snprintf(key, sizeof key, "colors.%s", keys[i]);
		if ((v = rros_conf_get(&c, key)))
			apply_color(p, keys[i], v);
		snprintf(key, sizeof key, "colors.%s.%s", dark ? "dark" : "light", keys[i]);
		if ((v = rros_conf_get(&c, key)))
			apply_color(p, keys[i], v);
	}
	rros_conf_free(&c);
	return 0;
}

static void mix(const char *a, const char *b, int pct_b, char out[8])
{
	unsigned pa = (unsigned)strtoul(a + 1, NULL, 16), pb = (unsigned)strtoul(b + 1, NULL, 16);
	unsigned q = (unsigned)pct_b, ch[3];
	int sh[3] = { 16, 8, 0 };

	for (int i = 0; i < 3; i++) {
		unsigned x = (pa >> sh[i]) & 255, y = (pb >> sh[i]) & 255;

		ch[i] = (x * (100 - q) + y * q) / 100;
	}
	snprintf(out, 8, "#%02x%02x%02x", ch[0], ch[1], ch[2]);
}

int rros_theme_palette(const struct rros_theme *t, struct rros_palette *p)
{
	int miss = load_palette(t->theme, t->dark, p, 0);

	if (miss)
		load_palette("lirio", t->dark, p, 0);
	if (t->accent[0])
		put(p->accent, t->accent);
	if (t->high_contrast) {
		/* alto contraste: negro y blanco puros, acento amarillo (o azul oscuro en claro) */
		static const struct rros_palette dark = { "#000000", "#000000", "#ffffff", "#f0f0f0", "#ffffff",
			"#ffff00", "", "#ff6b6b", "#5cff5c", "#ffff00", "#66ccff", "#ff99ff", "#66ffff" };
		static const struct rros_palette light = { "#ffffff", "#ffffff", "#000000", "#1a1a1a", "#000000",
			"#0000cc", "", "#b00000", "#006400", "#7a5c00", "#0000cc", "#800080", "#005f5f" };

		*p = t->dark ? dark : light;
	}
	mix(p->accent, p->bg, 55, p->accent_dim);
	return miss;
}

size_t rros_theme_list(struct rros_theme_info *out, size_t max)
{
	size_t n = 0;
	char dir[900];
	DIR *d;
	struct dirent *e;

	for (size_t i = 0; i < NBUILTIN && n < max; i++) {
		snprintf(out[n].name, sizeof out[n].name, "%s", builtin[i].name);
		out[n++].builtin = 1;
	}
	if (rros_themes_dir(dir, sizeof dir) || !(d = opendir(dir)))
		return n;
	while ((e = readdir(d)) && n < max) {
		size_t l = strlen(e->d_name);
		char name[48];

		if (l <= 5 || l - 5 >= sizeof name || strcmp(e->d_name + l - 5, ".toml"))
			continue;
		snprintf(name, sizeof name, "%.*s", (int)(l - 5), e->d_name);
		if (!rros_valid_profile_name(name) || find_builtin(name))
			continue;
		snprintf(out[n].name, sizeof out[n].name, "%.39s", name);
		out[n++].builtin = 0;
	}
	closedir(d);
	return n;
}

int rros_theme_new(const char *name, const char *base, char *err, size_t en)
{
	struct rros_palette dark, light;
	char dir[900], path[1000];
	FILE *f;

	if (!rros_valid_profile_name(name)) {
		snprintf(err, en, "nombre no válido (a-z, 0-9, _ o -, máx. 32)");
		return -1;
	}
	if (find_builtin(name)) {
		snprintf(err, en, "'%s' es un tema incluido; elige otro nombre", name);
		return -1;
	}
	if (rros_themes_dir(dir, sizeof dir)) {
		snprintf(err, en, "no se pudo determinar la carpeta de temas");
		return -1;
	}
	snprintf(path, sizeof path, "%s/%s.toml", dir, name);
	if (access(path, F_OK) == 0) {
		snprintf(err, en, "ya existe %s", path);
		return -1;
	}
	if (!rros_valid_profile_name(base) || load_palette(base, 1, &dark, 0) || load_palette(base, 0, &light, 0)) {
		snprintf(err, en, "el tema base '%s' no existe", base);
		return -1;
	}
	if (rros_mkdir_parents(path) || !(f = fopen(path, "w"))) {
		snprintf(err, en, "no se pudo crear %s", path);
		return -1;
	}
	fprintf(f, "# Tema propio de Lily. Cambia los colores (#RRGGBB) y aplica con: rros theme use %s\n", name);
	fprintf(f, "# Lo que no definas se hereda de '%s'.\n[theme]\nbase = \"%s\"\n\n", base, base);
#define V(k) fprintf(f, "%s = \"%s\"\n", #k, dark.k)
	fputs("[colors.dark]\n", f);
	V(bg); V(surface); V(text); V(muted); V(border); V(red); V(green); V(yellow); V(blue); V(magenta); V(cyan); V(accent);
#undef V
#define V(k) fprintf(f, "%s = \"%s\"\n", #k, light.k)
	fputs("\n[colors.light]\n", f);
	V(bg); V(surface); V(text); V(muted); V(border); V(red); V(green); V(yellow); V(blue); V(magenta); V(cyan); V(accent);
#undef V
	if (fclose(f) != 0) {
		snprintf(err, en, "no se pudo escribir %s", path);
		return -1;
	}
	return 0;
}

int rros_theme_set_color(const char *name, const char *variant, const char *key, const char *hex,
			 char *err, size_t en)
{
	static const char *keys[] = { "bg", "surface", "text", "muted", "border", "red", "green", "yellow",
				      "blue", "magenta", "cyan", "accent" };
	struct rros_conf c;
	char dir[900], path[1000], full[64];
	int known = 0, rc;

	if (!rros_valid_profile_name(name) || find_builtin(name)) {
		snprintf(err, en, "solo se pueden editar los temas propios");
		return -1;
	}
	if (strcmp(variant, "dark") && strcmp(variant, "light")) {
		snprintf(err, en, "la variante debe ser dark o light");
		return -1;
	}
	for (size_t i = 0; i < sizeof keys / sizeof *keys; i++)
		known |= !strcmp(keys[i], key);
	if (!known) {
		snprintf(err, en, "color desconocido '%s'", key);
		return -1;
	}
	if (!valid_hex(hex)) {
		snprintf(err, en, "el color debe ser #RRGGBB");
		return -1;
	}
	if (rros_themes_dir(dir, sizeof dir)) {
		snprintf(err, en, "no se pudo determinar la carpeta de temas");
		return -1;
	}
	snprintf(path, sizeof path, "%s/%s.toml", dir, name);
	rros_conf_init(&c);
	if (rros_conf_load(&c, path, NULL) != 0) {
		snprintf(err, en, "no existe el tema propio '%s'", name);
		rros_conf_free(&c);
		return -1;
	}
	snprintf(full, sizeof full, "colors.%s.%s", variant, key);
	rc = rros_conf_set(&c, full, hex, 1) || rros_conf_save(&c, path);
	rros_conf_free(&c);
	if (rc) {
		snprintf(err, en, "no se pudo escribir %s", path);
		return -1;
	}
	return 0;
}
