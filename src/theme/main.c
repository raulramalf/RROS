/* rros-theme: genera la configuración del escritorio RROS desde rros-config. */
#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/wait.h>
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
	{ "hypr/hyprland.conf", rros_render_hyprland },
	{ "hypr/hyprlock.conf", rros_render_hyprlock },
	{ "kde/RROS.colors", rros_render_kde_colors },
	{ "kxkbrc", rros_render_kxkbrc },
};

static void usage(void)
{
	fputs("uso: rros-theme [-p PERFIL] COMANDO\n"
	      "  list                   temas disponibles (* = en uso)\n"
	      "  use NOMBRE             cambia de tema y lo aplica     rros-theme use nord\n"
	      "  cycle                  pasa al siguiente tema\n"
	      "  toggle                 alterna oscuro/claro\n"
	      "  new NOMBRE [--base T]  crea un tema propio para editar\n"
	      "  apply [--quiet]        escribe ~/.config/{hypr,sway,waybar,foot,mako,fuzzel,gtk-*} y recarga\n"
	      "  escritorio [ESTILO]    estilos de escritorio (clasico, moderno, mosaico); con ESTILO lo aplica\n"
	      "  wallpaper [--next | --style S] [--out F] [--size AxB]   genera el fondo con los colores del tema\n"
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
	if (rc) {
		rros_conf_free(c);
		return rc;
	}
	/* el compositor que de verdad corre (rros-session cae a sway si no hay aceleración gráfica) */
	{
		const char *sess = getenv("RROS_SESSION");

		if (sess && !strcmp(sess, "sway"))
			t->hyprland = 0;
		else if (sess && !strcmp(sess, "hyprland"))
			t->hyprland = 1;
		else if (getenv("HYPRLAND_INSTANCE_SIGNATURE"))
			t->hyprland = 1;
		else if (getenv("SWAYSOCK"))
			t->hyprland = 0;
	}
	return 0;
}

/* genera el fondo del tema si hace falta (el nombre del archivo cambia con los colores) */
static void ensure_wallpaper(const struct rros_theme *t, const struct rros_palette *p, char *path, size_t n)
{
	rros_wallpaper_path(t, p, path, n);
	if (!path[0] || t->wallpaper[0] || access(path, R_OK) == 0)
		return;
	if (rros_mkdir_parents(path) || rros_wallpaper_render(t->wallpaper_style, p, 1920, 1080, path)) {
		fprintf(stderr, "rros-theme: no se pudo generar el fondo %s\n", path);
		path[0] = '\0';
	}
}

/* KDE Plasma: el esquema de colores va a ~/.local/share/color-schemes con un nombre que cambia con los colores
 * (plasma-apply-colorscheme no recarga un esquema con el mismo nombre). Si Plasma está instalado, se aplica; en
 * una sesión de Plasma, también el fondo. */
static void apply_plasma(const struct rros_theme *t, const struct rros_palette *p, const struct rros_conf *c,
			 const char *wallpaper, int quiet)
{
	const char *xd = getenv("XDG_DATA_HOME"), *home = getenv("HOME"), *desk = getenv("XDG_CURRENT_DESKTOP");
	char dir[700], path[800], name[32], cmd[1200];
	unsigned long h = 2166136261UL;
	const char *const *col = (const char *const[]){ p->bg, p->surface, p->text, p->muted, p->accent, p->red,
							p->green, p->yellow, p->blue, p->magenta, NULL };
	FILE *f;
	DIR *d;
	struct dirent *e;

	if (access("/usr/bin/plasma-apply-colorscheme", X_OK) != 0 && !getenv("RROS_PLASMA_TEST"))
		return;
	if (xd && *xd)
		snprintf(dir, sizeof dir, "%s/color-schemes", xd);
	else if (home && *home)
		snprintf(dir, sizeof dir, "%s/.local/share/color-schemes", home);
	else
		return;
	for (; *col; col++)
		for (const char *q = *col; *q; q++)
			h = (h ^ (unsigned char)*q) * 16777619UL;
	snprintf(name, sizeof name, "RROS-%08lx", h & 0xffffffffUL);
	snprintf(path, sizeof path, "%s/%s.colors", dir, name);
	if (rros_mkdir_parents(path) || !(f = fopen(path, "w"))) {
		fprintf(stderr, "rros-theme: no se pudo escribir %s\n", path);
		return;
	}
	rros_render_kde_colors(t, p, c, f);
	fclose(f);
	/* fuera los esquemas de RROS anteriores */
	if ((d = opendir(dir))) {
		while ((e = readdir(d)))
			if (!strncmp(e->d_name, "RROS-", 5) && strcmp(e->d_name + strlen(e->d_name) - 7, ".colors") == 0 &&
			    strncmp(e->d_name, name, strlen(name))) {
				char old[1100];

				snprintf(old, sizeof old, "%s/%s", dir, e->d_name);
				unlink(old);
			}
		closedir(d);
	}
	/* las barras de título, con los mismos colores (KWin lee primero las de ~/.local/share) */
	snprintf(path, sizeof path, "%s/../aurorae/themes/RROS", dir);
	if (rros_write_aurorae(path, t, p) != 0)
		fputs("rros-theme: no se pudo escribir el tema de las barras de título\n", stderr);
	if (getenv("RROS_PLASMA_TEST"))
		return;
	/* KWin vuelve a leer la decoración */
	if (system("qdbus6 org.kde.KWin /KWin reconfigure >/dev/null 2>&1 || "
		   "dbus-send --session --type=method_call --dest=org.kde.KWin /KWin org.kde.KWin.reconfigure >/dev/null 2>&1") != 0 &&
	    !quiet && desk && strstr(desk, "KDE"))
		fputs("rros-theme: aviso: no se pudo recargar KWin\n", stderr);
	snprintf(cmd, sizeof cmd, "plasma-apply-colorscheme %s >/dev/null 2>&1", name);
	if (system(cmd) != 0 && !quiet)
		fputs("rros-theme: aviso: plasma-apply-colorscheme falló\n", stderr);
	if (desk && strstr(desk, "KDE") && wallpaper[0] && !strchr(wallpaper, '\'')) {
		snprintf(cmd, sizeof cmd, "plasma-apply-wallpaperimage '%s' >/dev/null 2>&1", wallpaper);
		if (system(cmd) != 0 && !quiet)
			fputs("rros-theme: aviso: plasma-apply-wallpaperimage falló\n", stderr);
	}
}

/* recarga lo que está en marcha: compositor, fondo, panel y notificaciones */
static void reload_desktop(const struct rros_theme *t, const char *wallpaper, int quiet)
{
	int bad = 0;

	if (getenv("HYPRLAND_INSTANCE_SIGNATURE")) {
		char cmd[700];

		bad |= system("hyprctl reload >/dev/null 2>&1") != 0;
		if (wallpaper[0])
			snprintf(cmd, sizeof cmd, "pkill -x swaybg; setsid swaybg -i '%s' -m %s >/dev/null 2>&1 &", wallpaper,
				 t->wallpaper[0] ? t->wallpaper_mode : "fill");
		else
			snprintf(cmd, sizeof cmd, "pkill -x swaybg; setsid swaybg -c '#000000' >/dev/null 2>&1 &");
		bad |= system(cmd) != 0;
	} else if (getenv("SWAYSOCK")) {
		bad |= system("swaymsg -q reload >/dev/null 2>&1") != 0;
	} else {
		return;
	}
	if (system("pkill -SIGUSR2 waybar >/dev/null 2>&1; makoctl reload >/dev/null 2>&1 || pkill -SIGUSR2 mako >/dev/null 2>&1") != 0)
		bad = 1;
	if (bad && !quiet)
		fputs("rros-theme: aviso: no se pudo recargar todo el escritorio\n", stderr);
}

static int cmd_apply(const char *profile, int quiet)
{
	struct rros_theme t;
	struct rros_conf c;
	struct rros_palette p;
	char base[1024], wp[300];
	int rc;

	if (load_theme(&t, &c, profile))
		return 1;
	if (config_home(base, sizeof base)) {
		fputs("rros-theme: no se pudo determinar ~/.config\n", stderr);
		rros_conf_free(&c);
		return 1;
	}
	rros_theme_palette(&t, &p);
	{
		/* el archivo propio del usuario para Hyprland: existe siempre, así `source` no falla */
		char personal[1100];
		FILE *pf;

		snprintf(personal, sizeof personal, "%s/hypr/personal.conf", base);
		if (access(personal, F_OK) != 0 && rros_mkdir_parents(personal) == 0 && (pf = fopen(personal, "w"))) {
			fputs("# Tus líneas para Hyprland (https://wiki.hypr.land). RROS no toca este archivo.\n", pf);
			fclose(pf);
		}
	}
	/* primero la configuración (rápido) y después el fondo, que la primera vez tarda medio segundo */
	rc = render_all(base, &t, &c, access("/usr/bin/swayfx", X_OK) == 0);
	ensure_wallpaper(&t, &p, wp, sizeof wp);
	if (rc == 0) {
		reload_desktop(&t, wp, quiet);
		apply_plasma(&t, &p, &c, wp, quiet);
	}
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

/* En una sesión de Plasma, el estilo cambia también los paneles: plasmashell ejecuta el guion del estilo
 * (/usr/share/rros/plasma/ESTILO.js) por D-Bus, sin shell de por medio. */
static void plasma_layout(const char *id)
{
	const char *desk = getenv("XDG_CURRENT_DESKTOP"), *dir = getenv("RROS_PLASMA_LAYOUTS");
	char path[600], *script;
	FILE *f;
	long len;
	pid_t pid;
	int st;

	if (!desk || !strstr(desk, "KDE"))
		return;
	snprintf(path, sizeof path, "%s/%s.js", dir && *dir ? dir : "/usr/share/rros/plasma", id);
	if (!(f = fopen(path, "r")))
		return;
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	rewind(f);
	script = len > 0 && len < 65536 ? calloc(1, (size_t)len + 1) : NULL;
	if (!script || fread(script, 1, (size_t)len, f) != (size_t)len) {
		fclose(f);
		free(script);
		return;
	}
	fclose(f);
	pid = fork();
	if (pid == 0) {
		execlp("qdbus6", "qdbus6", "org.kde.plasmashell", "/PlasmaShell", "org.kde.PlasmaShell.evaluateScript",
		       script, (char *)NULL);
		_exit(127);
	}
	if (pid < 0 || waitpid(pid, &st, 0) < 0 || !WIFEXITED(st) || WEXITSTATUS(st) != 0)
		fputs("rros-theme: aviso: Plasma no pudo cambiar los paneles\n", stderr);
	free(script);
}

static int cmd_desktop(const char *profile, const char *name)
{
	struct rros_theme t;
	struct rros_conf c;
	const struct rros_desktop_preset *cur, *d;
	char err[300];

	if (!name) {
		if (load_theme(&t, &c, profile))
			return 1;
		cur = rros_desktop_current(&c);
		for (size_t i = 0; i < rros_desktop_presets_len; i++) {
			d = &rros_desktop_presets[i];
			printf("%c %-9s %s\n", d == cur ? '*' : ' ', d->id, d->desc);
		}
		if (!cur)
			puts("  (ahora: personalizado)");
		rros_conf_free(&c);
		return 0;
	}
	d = rros_desktop_find(name);
	if (!d) {
		fprintf(stderr, "rros-theme: no hay ningún estilo de escritorio '%s' (clasico, moderno, mosaico)\n", name);
		return 1;
	}
	for (size_t k = 0; k < 12 && d->keys[k][0]; k++)
		if (rros_user_set(d->keys[k][0], d->keys[k][1], 0, err, sizeof err)) {
			fprintf(stderr, "rros-theme: %s\n", err);
			return 1;
		}
	printf("escritorio: %s\n", d->label);
	plasma_layout(d->id);
	return cmd_apply(profile, 1);
}

static int cmd_wallpaper(const char *profile, int argc, char **argv, int i)
{
	static const char *styles[] = { "aurora", "ondas", "degradado", "circulos", "liso" };
	const char *style = NULL, *out = NULL;
	int w = 1920, h = 1080, next = 0;
	struct rros_theme t;
	struct rros_conf c;
	struct rros_palette p;
	char err[300];

	for (; i < argc; i++) {
		if (!strcmp(argv[i], "--style") && i + 1 < argc)
			style = argv[++i];
		else if (!strcmp(argv[i], "--out") && i + 1 < argc)
			out = argv[++i];
		else if (!strcmp(argv[i], "--size") && i + 1 < argc && sscanf(argv[++i], "%dx%d", &w, &h) == 2)
			;
		else if (!strcmp(argv[i], "--next"))
			next = 1;
		else {
			usage();
			return 2;
		}
	}
	if (load_theme(&t, &c, profile))
		return 1;
	rros_conf_free(&c);
	rros_theme_palette(&t, &p);
	if (next) {
		size_t k = 0;

		while (k < 4 && strcmp(styles[k], t.wallpaper_style))
			k++;
		if (rros_user_set("ui.wallpaper_style", styles[(k + 1) % 4], 0, err, sizeof err) ||
		    (t.wallpaper[0] && rros_user_unset("ui.wallpaper", 0, err, sizeof err))) {
			fprintf(stderr, "rros-theme: %s\n", err);
			return 1;
		}
		printf("fondo: %s\n", styles[(k + 1) % 4]);
		return cmd_apply(profile, 1);
	}
	if (!style)
		style = t.wallpaper_style;
	if (!out) {
		fputs("rros-theme: indica --out ARCHIVO.png (o usa --next para cambiar el del escritorio)\n", stderr);
		return 2;
	}
	{
		size_t k = 0;

		while (k < 4 && strcmp(styles[k], style))
			k++;
		if (k == 4) {
			fprintf(stderr, "rros-theme: estilo de fondo desconocido: %s (aurora, ondas, degradado, circulos)\n", style);
			return 1;
		}
	}
	if (rros_wallpaper_render(style, &p, w, h, out)) {
		fprintf(stderr, "rros-theme: no se pudo escribir %s\n", out);
		return 1;
	}
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
	if (!strcmp(cmd, "escritorio") || !strcmp(cmd, "desktop"))
		return cmd_desktop(profile, i < argc ? argv[i] : NULL);
	if (!strcmp(cmd, "wallpaper") || !strcmp(cmd, "fondo"))
		return cmd_wallpaper(profile, argc, argv, i);
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
