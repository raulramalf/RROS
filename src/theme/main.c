/* rros-theme: genera la configuración del escritorio RROS desde rros-config. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "rros_profile.h"
#include "rros_userconf.h"
#include "theme.h"

struct target {
	const char *rel;
	rros_render_fn fn;
};

static const struct target targets[] = {
	{ "sway/config", rros_render_sway },
	{ "sway/rros-effects.conf", rros_render_effects },
	{ "waybar/config", rros_render_waybar },
	{ "waybar/style.css", rros_render_waybar_css },
	{ "foot/foot.ini", rros_render_foot },
	{ "mako/config", rros_render_mako },
	{ "fuzzel/fuzzel.ini", rros_render_fuzzel },
	{ "swaylock/config", rros_render_swaylock },
	{ "gtk-3.0/settings.ini", rros_render_gtk_settings },
	{ "gtk-3.0/gtk.css", rros_render_gtk_css },
	{ "gtk-4.0/settings.ini", rros_render_gtk_settings },
	{ "gtk-4.0/gtk.css", rros_render_gtk_css },
};

static void usage(void)
{
	fputs("uso: rros-theme [-p PERFIL] COMANDO\n"
	      "  list                   temas disponibles (* = en uso)\n"
	      "  use NOMBRE             cambia de tema y lo aplica     rros-theme use nord\n"
	      "  cycle                  pasa al siguiente tema\n"
	      "  toggle                 alterna oscuro/claro\n"
	      "  new NOMBRE [--base T]  crea un tema propio para editar\n"
	      "  apply [--quiet]        escribe ~/.config/{sway,waybar,foot,mako,fuzzel,gtk-*} y recarga\n"
	      "  palette                muestra la paleta calculada\n"
	      "  render --out DIR [--swayfx]   escribe en DIR (pruebas)\n", stderr);
}

static int has_marker(const char *path)
{
	char buf[256];
	FILE *f = fopen(path, "r");
	size_t n;

	if (!f)
		return 0;
	n = fread(buf, 1, sizeof buf - 1, f);
	fclose(f);
	buf[n] = '\0';
	return strstr(buf, RROS_GENERATED) != NULL;
}

static int write_target(const char *base, const struct target *tg, const struct rros_theme *t,
			const struct rros_palette *p, const struct rros_conf *conf, int effects_enabled)
{
	char path[1100], tmp[1110], bak[1110];
	struct stat st;
	FILE *f;

	snprintf(path, sizeof path, "%s/%s", base, tg->rel);
	snprintf(tmp, sizeof tmp, "%s.tmp", path);
	snprintf(bak, sizeof bak, "%s.bak-rros", path);
	if (rros_mkdir_parents(path)) {
		fprintf(stderr, "rros-theme: no se pudo crear el directorio de %s\n", path);
		return -1;
	}
	if (stat(path, &st) == 0 && !has_marker(path)) {
		if (stat(bak, &st) == 0) {
			fprintf(stderr, "rros-theme: %s es manual y ya existe copia; no se sobrescribe\n", path);
			return -1;
		}
		if (rename(path, bak) != 0)
			return -1;
		fprintf(stderr, "rros-theme: copia de tu archivo en %s\n", bak);
	}
	f = fopen(tmp, "w");
	if (!f) {
		fprintf(stderr, "rros-theme: %s: %s\n", tmp, strerror(errno));
		return -1;
	}
	if (tg->fn == rros_render_effects && !effects_enabled)
		fprintf(f, "# %s (sin SwayFX instalado: sin efectos).\n", RROS_GENERATED);
	else
		tg->fn(t, p, conf, f);
	if (fclose(f) != 0 || rename(tmp, path) != 0) {
		remove(tmp);
		return -1;
	}
	return 0;
}

static int render_all(const char *base, const struct rros_theme *t, const struct rros_conf *conf, int swayfx)
{
	struct rros_palette p;
	int bad = 0;

	if (rros_theme_palette(t, &p))
		fprintf(stderr, "rros-theme: el tema '%s' no existe; se usa 'aurora' (rros-theme list)\n", t->theme);
	for (size_t i = 0; i < sizeof targets / sizeof *targets; i++)
		bad |= write_target(base, &targets[i], t, &p, conf, swayfx) != 0;
	return bad ? 1 : 0;
}

static int config_home(char *buf, size_t n)
{
	const char *x = getenv("XDG_CONFIG_HOME"), *h = getenv("HOME");
	int len;

	if (x && *x)
		len = snprintf(buf, n, "%s", x);
	else if (h && *h)
		len = snprintf(buf, n, "%s/.config", h);
	else
		return -1;
	return (len < 0 || (size_t)len >= n) ? -1 : 0;
}

static int load_theme(struct rros_theme *t, struct rros_conf *c, const char *profile)
{
	int rc;

	rros_conf_init(c);
	rc = rros_load_effective(c, profile);
	if (rc == -1)
		fprintf(stderr, "rros-theme: perfil '%s' no encontrado\n", profile);
	else if (rc)
		fputs("rros-theme: configuración ilegible (rros-config validate)\n", stderr);
	else
		rros_theme_from_conf(t, c);
	if (rc)
		rros_conf_free(c);
	return rc;
}

static int cmd_apply(const char *profile, int quiet)
{
	struct rros_theme t;
	struct rros_conf c;
	struct rros_palette p;
	char base[1024];
	int rc;

	if (load_theme(&t, &c, profile))
		return 1;
	if (config_home(base, sizeof base)) {
		fputs("rros-theme: no se pudo determinar ~/.config\n", stderr);
		rros_conf_free(&c);
		return 1;
	}
	rc = render_all(base, &t, &c, t.full_compositor && access("/usr/bin/swayfx", X_OK) == 0);
	rros_theme_palette(&t, &p);
	if (rc == 0 && getenv("SWAYSOCK"))
		if (system("swaymsg -q reload >/dev/null 2>&1; pkill -SIGUSR2 waybar >/dev/null 2>&1; pkill -SIGUSR2 mako >/dev/null 2>&1") != 0 && !quiet)
			fputs("rros-theme: aviso: no se pudo recargar el escritorio\n", stderr);
	if (rc == 0 && !quiet)
		printf("tema aplicado: %s (%s, acento %s) en %s\n", t.theme, t.dark ? "oscuro" : "claro", p.accent, base);
	rros_conf_free(&c);
	return rc;
}

static int cmd_toggle(const char *profile)
{
	struct rros_theme t;
	struct rros_conf c;
	char err[300];

	if (load_theme(&t, &c, profile))
		return 1;
	rros_conf_free(&c);
	if (rros_user_set("ui.color_scheme", t.dark ? "light" : "dark", 0, err, sizeof err)) {
		fprintf(stderr, "rros-theme: %s\n", err);
		return 1;
	}
	return cmd_apply(profile, 0);
}

static int theme_exists(const char *name)
{
	struct rros_theme_info list[64];
	size_t n = rros_theme_list(list, 64);

	for (size_t i = 0; i < n; i++)
		if (!strcmp(list[i].name, name))
			return 1;
	return 0;
}

static int cmd_use(const char *profile, const char *name)
{
	char err[300];

	if (!theme_exists(name)) {
		fprintf(stderr, "rros-theme: el tema '%s' no existe (mira `rros-theme list`)\n", name);
		return 1;
	}
	if (rros_user_set("ui.theme", name, 0, err, sizeof err)) {
		fprintf(stderr, "rros-theme: %s\n", err);
		return 1;
	}
	return cmd_apply(profile, 0);
}

static int cmd_list(const char *profile)
{
	struct rros_theme_info list[64];
	struct rros_theme t;
	struct rros_conf c;
	size_t n = rros_theme_list(list, 64);
	int have = load_theme(&t, &c, profile) == 0;

	for (size_t i = 0; i < n; i++) {
		printf("%c %-16s %s\n", have && !strcmp(list[i].name, t.theme) ? '*' : ' ', list[i].name,
		       list[i].builtin ? "incluido" : "propio");
	}
	if (have)
		rros_conf_free(&c);
	return 0;
}

static int cmd_cycle(const char *profile)
{
	struct rros_theme_info list[64];
	struct rros_theme t;
	struct rros_conf c;
	size_t n = rros_theme_list(list, 64), cur = 0;

	if (load_theme(&t, &c, profile))
		return 1;
	rros_conf_free(&c);
	for (size_t i = 0; i < n; i++)
		if (!strcmp(list[i].name, t.theme))
			cur = i;
	return cmd_use(profile, list[(cur + 1) % n].name);
}

int main(int argc, char **argv)
{
	char profile[64] = "";
	int i = 1;
	struct rros_theme t;
	struct rros_conf conf;
	const char *cmd;

	if (i + 1 < argc && !strcmp(argv[i], "-p")) {
		snprintf(profile, sizeof profile, "%s", argv[i + 1]);
		i += 2;
	}
	if (i >= argc) {
		usage();
		return 2;
	}
	cmd = argv[i++];
	if (!*profile)
		rros_read_active_profile(profile, sizeof profile);

	if (!strcmp(cmd, "apply"))
		return cmd_apply(profile, i < argc && !strcmp(argv[i], "--quiet"));
	if (!strcmp(cmd, "toggle"))
		return cmd_toggle(profile);
	if (!strcmp(cmd, "list"))
		return cmd_list(profile);
	if (!strcmp(cmd, "cycle"))
		return cmd_cycle(profile);
	if (!strcmp(cmd, "use") && i + 1 == argc)
		return cmd_use(profile, argv[i]);
	if (!strcmp(cmd, "new") && i < argc) {
		const char *name = argv[i], *base = "aurora";
		char err[300];

		for (i++; i < argc; i++) {
			if (!strcmp(argv[i], "--base") && i + 1 < argc)
				base = argv[++i];
			else {
				usage();
				return 2;
			}
		}
		if (rros_theme_new(name, base, err, sizeof err)) {
			fprintf(stderr, "rros-theme: %s\n", err);
			return 1;
		}
		{
			char dir[900];

			rros_themes_dir(dir, sizeof dir);
			printf("creado %s/%s.toml\nedítalo y actívalo con: rros-theme use %s\n", dir, name, name);
		}
		return 0;
	}
	if (!strcmp(cmd, "palette")) {
		struct rros_palette p;

		if (load_theme(&t, &conf, profile))
			return 1;
		rros_conf_free(&conf);
		rros_theme_palette(&t, &p);
		printf("bg=%s\nsurface=%s\ntext=%s\nmuted=%s\nborder=%s\naccent=%s\naccent_dim=%s\n"
		       "red=%s\ngreen=%s\nyellow=%s\nblue=%s\nmagenta=%s\ncyan=%s\n", p.bg, p.surface,
		       p.text, p.muted, p.border, p.accent, p.accent_dim, p.red, p.green, p.yellow,
		       p.blue, p.magenta, p.cyan);
		return 0;
	}
	if (!strcmp(cmd, "render")) {
		const char *out = NULL;
		int swayfx = 0;

		for (; i < argc; i++) {
			if (!strcmp(argv[i], "--out") && i + 1 < argc)
				out = argv[++i];
			else if (!strcmp(argv[i], "--swayfx"))
				swayfx = 1;
			else {
				usage();
				return 2;
			}
		}
		if (!out) {
			usage();
			return 2;
		}
		if (load_theme(&t, &conf, profile))
			return 1;
		i = render_all(out, &t, &conf, swayfx);
		rros_conf_free(&conf);
		return i;
	}
	usage();
	return 2;
}
