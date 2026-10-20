/* rros-theme: genera la configuración del escritorio RROS desde rros-config. */
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
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

/* archivos enteros que escribe RROS (relativos a ~/.local/share, o a la carpeta de «render») */
static const struct target targets[] = {
	{ "konsole/RROS.profile", rros_render_konsole_profile },
	{ "konsole/RROS.colorscheme", rros_render_konsole_colors },
};

static void usage(void)
{
	fputs("uso: rros-theme [-p PERFIL] COMANDO\n"
	      "  list                   temas disponibles (* = en uso)\n"
	      "  use NOMBRE             cambia de tema y lo aplica     rros-theme use nord\n"
	      "  cycle                  pasa al siguiente tema\n"
	      "  toggle                 alterna oscuro/claro\n"
	      "  new NOMBRE [--base T]  crea un tema propio para editar\n"
	      "  apply [--quiet]        aplica la configuración a Plasma (colores, dock, efectos, entrada...)\n"
	      "  escritorio [ESTILO]    estilos de escritorio (rros, clasico, isla, moderno, doble, lateral); con ESTILO lo aplica\n"
	      "  wallpaper [--next | --style S] [--out F] [--size AxB]   genera el fondo con los colores del tema\n"
	      "  palette                muestra la paleta calculada\n"
	      "  render --out DIR       escribe en DIR todo lo que se generaría (pruebas)\n", stderr);
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
			const struct rros_palette *p, const struct rros_conf *conf)
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
	tg->fn(t, p, conf, f);
	if (fclose(f) != 0 || rename(tmp, path) != 0) {
		remove(tmp);
		return -1;
	}
	return 0;
}

/* archivos propios (Konsole) en data y ediciones de los archivos de KDE en cfg */
static int render_all(const char *cfg, const char *data, const struct rros_theme *t, const struct rros_conf *conf)
{
	static struct rros_ini_edit ed[RROS_MAX_EDITS];
	struct rros_palette p;
	size_t ne;
	int bad = 0;

	if (rros_theme_palette(t, &p))
		fprintf(stderr, "rros-theme: el tema '%s' no existe; se usa 'lirio' (rros-theme list)\n", t->theme);
	for (size_t i = 0; i < sizeof targets / sizeof *targets; i++)
		bad |= write_target(data, &targets[i], t, &p, conf) != 0;
	ne = rros_plasma_edits(t, &p, conf, ed, RROS_MAX_EDITS);
	for (const char *const *f = rros_kde_files; *f; f++) {
		char path[1100];

		snprintf(path, sizeof path, "%s/%s", cfg, *f);
		if (rros_ini_apply(path, *f, ed, ne)) {
			fprintf(stderr, "rros-theme: no se pudo escribir %s\n", path);
			bad = 1;
		}
	}
	{
		char apps[1100], autos[1100];

		snprintf(apps, sizeof apps, "%s/applications", data);
		snprintf(autos, sizeof autos, "%s/autostart", cfg);
		bad |= rros_write_launchers(conf, apps, autos) != 0;
	}
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

static int data_home(char *buf, size_t n)
{
	const char *x = getenv("XDG_DATA_HOME"), *h = getenv("HOME");
	int len;

	if (x && *x)
		len = snprintf(buf, n, "%s", x);
	else if (h && *h)
		len = snprintf(buf, n, "%s/.local/share", h);
	else
		return -1;
	return (len < 0 || (size_t)len >= n) ? -1 : 0;
}

static int state_home(char *buf, size_t n)
{
	const char *x = getenv("XDG_STATE_HOME"), *h = getenv("HOME");
	int len;

	if (x && *x)
		len = snprintf(buf, n, "%s/rros", x);
	else if (h && *h)
		len = snprintf(buf, n, "%s/.local/state/rros", h);
	else
		return -1;
	return (len < 0 || (size_t)len >= n) ? -1 : 0;
}

static int in_plasma(void)
{
	const char *desk = getenv("XDG_CURRENT_DESKTOP");

	return desk && strstr(desk, "KDE") && !getenv("RROS_PLASMA_TEST");
}

/* RROS Shell (barra, dock, Launchpad...) en lugar de plasmashell: lee la configuración y el fondo por su cuenta.
 * RROS_SHELL=0|1 lo fuerza (pruebas). */
static int rros_shell(void)
{
	const char *e = getenv("RROS_SHELL");

	if (e)
		return !strcmp(e, "1");
	return access("/usr/bin/rros-shell", X_OK) == 0;
}

/* lanza un programa sin shell y espera; con out, guarda su salida (hasta n bytes). Devuelve el código de salida. */
static int run(char *const argv[], char *out, size_t n)
{
	int fd[2] = { -1, -1 }, st;
	pid_t pid;

	if (out && pipe(fd))
		return -1;
	pid = fork();
	if (pid == 0) {
		int nul = open("/dev/null", O_RDWR);

		if (nul >= 0) {
			dup2(nul, 0);
			dup2(out ? fd[1] : nul, 1);
			dup2(nul, 2);
		}
		if (out) {
			close(fd[0]);
			close(fd[1]);
		}
		execvp(argv[0], argv);
		_exit(127);
	}
	if (out) {
		size_t got = 0;
		ssize_t k;

		close(fd[1]);
		while (pid > 0 && (k = read(fd[0], out + got, n - 1 - got > 0 ? n - 1 - got : 0)) > 0)
			got += (size_t)k;
		/* lo que no cabe, se descarta */
		while (pid > 0 && got >= n - 1 && read(fd[0], (char[512]){ 0 }, 512) > 0)
			;
		out[got] = '\0';
		close(fd[0]);
	}
	if (pid < 0 || waitpid(pid, &st, 0) < 0)
		return -1;
	return WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}

/* Sin aceleración gráfica (máquina virtual sin 3D), KWin dibuja con la CPU (llvmpipe): con efectos, todo va a
 * tirones. RROS_SOFT_RENDER=1|0 lo fuerza (pruebas). */
static int soft_render(void)
{
	const char *e = getenv("RROS_SOFT_RENDER");
	static char info[65536];
	char *argv[] = { "qdbus6", "org.kde.KWin", "/KWin", "supportInformation", NULL };

	if (e)
		return !strcmp(e, "1");
	if (!in_plasma() || run(argv, info, sizeof info) != 0)
		return 0;
	return strstr(info, "llvmpipe") || strstr(info, "softpipe") || strstr(info, "QPainter");
}

static int read_file(const char *path, char *buf, size_t n)
{
	FILE *f = fopen(path, "r");
	size_t k;

	buf[0] = '\0';
	if (!f)
		return -1;
	k = fread(buf, 1, n - 1, f);
	buf[k] = '\0';
	fclose(f);
	return 0;
}

static int write_text(const char *path, const char *text)
{
	FILE *f;

	if (rros_mkdir_parents(path) || !(f = fopen(path, "w")))
		return -1;
	fputs(text, f);
	return fclose(f);
}

/* El guion de paneles se ejecuta en plasmashell solo si cambió (rehace la barra y el dock). La copia de la
 * última vez queda en ~/.local/state/rros; la ISO trae la del perfil, así el primer inicio no rehace nada. */
static void plasma_layout(const struct rros_theme *t, const struct rros_palette *p, const struct rros_conf *c, int force)
{
	static char script[65536], prev[65536];
	char st[900], path[1000];
	FILE *f = fmemopen(script, sizeof script, "w");
	char *argv[] = { "qdbus6", "org.kde.plasmashell", "/PlasmaShell", "org.kde.PlasmaShell.evaluateScript", script, NULL };

	if (rros_shell()) {
		if (f)
			fclose(f);
		return;
	}
	if (!f)
		return;
	rros_render_plasma_layout(t, p, c, f);
	fclose(f);
	if (state_home(st, sizeof st))
		return;
	snprintf(path, sizeof path, "%s/plasma-layout.js", st);
	read_file(path, prev, sizeof prev);
	if (!force && !strcmp(prev, script))
		return;
	if (!in_plasma())
		return;
	if (run(argv, NULL, 0) != 0) {
		fputs("rros-theme: aviso: Plasma no pudo cambiar la barra y el dock\n", stderr);
		return;
	}
	write_text(path, script);
}

/* KDE Plasma: el esquema de colores va a ~/.local/share/color-schemes con un nombre que cambia con los colores
 * (plasma-apply-colorscheme no recarga un esquema con el mismo nombre); las barras de título, a aurorae. */
static void plasma_colors(const struct rros_theme *t, const struct rros_palette *p, const struct rros_conf *c,
			  const char *data, const char *cfg, int quiet)
{
	char dir[1100], path[1200], name[32], cur[64], kg[1100];
	unsigned long h = 2166136261UL;
	const char *const *col = (const char *const[]){ p->bg, p->surface, p->text, p->muted, p->accent, p->red,
							p->green, p->yellow, p->blue, p->magenta, NULL };
	FILE *f;
	DIR *d;
	struct dirent *e;

	snprintf(dir, sizeof dir, "%s/color-schemes", data);
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
			if (!strncmp(e->d_name, "RROS-", 5) && strlen(e->d_name) > 7 &&
			    !strcmp(e->d_name + strlen(e->d_name) - 7, ".colors") && strncmp(e->d_name, name, strlen(name))) {
				char old[1400];

				snprintf(old, sizeof old, "%s/%s", dir, e->d_name);
				unlink(old);
			}
		closedir(d);
	}
	snprintf(path, sizeof path, "%s/aurorae/themes/RROS", data);
	if (rros_write_aurorae(path, t, p) != 0)
		fputs("rros-theme: no se pudo escribir el tema de las barras de título\n", stderr);
	snprintf(kg, sizeof kg, "%s/kdeglobals", cfg);
	rros_ini_get(kg, "General", "ColorScheme", cur, sizeof cur);
	if (in_plasma() && strcmp(cur, name)) {
		char *argv[] = { "plasma-apply-colorscheme", name, NULL };

		if (run(argv, NULL, 0) != 0 && !quiet)
			fputs("rros-theme: aviso: plasma-apply-colorscheme falló\n", stderr);
	}
}

/* Plasma vuelve a leer lo que ha cambiado */
static void plasma_reload(const struct rros_theme *t, const char *old_theme, const char *wallpaper, const char *st)
{
	char *kwin[] = { "qdbus6", "org.kde.KWin", "/KWin", "reconfigure", NULL };
	char *glob[] = { "dbus-send", "--session", "--type=signal", "/KGlobalSettings", "org.kde.KGlobalSettings.notifyChange",
			 "int32:2", "int32:0", NULL };
	char *kbd[] = { "dbus-send", "--session", "--type=signal", "/Layouts", "org.kde.keyboard.reloadConfig", NULL };
	char *lock[] = { "qdbus6", "org.kde.screensaver", "/ScreenSaver", "configure", NULL };
	char *pw[] = { "qdbus6", "org.kde.Solid.PowerManagement", "/org/kde/Solid/PowerManagement", "reparseConfiguration", NULL };
	char *shell[] = { "qdbus6", "org.rros.Shell", "/Shell", "org.rros.Shell.reload", NULL };
	char wpfile[1100], prev[400];

	snprintf(wpfile, sizeof wpfile, "%s/wallpaper", st);
	/* RROS Shell vigila este archivo: el fondo cambia en cuanto se escribe */
	if (rros_shell() && wallpaper[0]) {
		read_file(wpfile, prev, sizeof prev);
		if (strcmp(prev, wallpaper))
			write_text(wpfile, wallpaper);
	}
	if (!in_plasma())
		return;
	run(kwin, NULL, 0);
	run(glob, NULL, 0);
	run(kbd, NULL, 0);
	run(lock, NULL, 0);
	run(pw, NULL, 0);
	if (strcmp(old_theme, rros_desktoptheme(t))) {
		char *dt[] = { "plasma-apply-desktoptheme", (char *)rros_desktoptheme(t), NULL };

		run(dt, NULL, 0);
	}
	if (rros_shell()) {
		run(shell, NULL, 0);
		return;
	}
	/* el fondo, solo si es otro */
	read_file(wpfile, prev, sizeof prev);
	if (wallpaper[0] && strcmp(prev, wallpaper)) {
		char *wp[] = { "plasma-apply-wallpaperimage", (char *)wallpaper, NULL };

		if (run(wp, NULL, 0) == 0)
			write_text(wpfile, wallpaper);
	}
}

static int cmd_apply_ex(const char *profile, int quiet, int force_layout)
{
	struct rros_theme t;
	struct rros_conf c;
	struct rros_palette p;
	char cfg[1024], data[1024], st[1024], wp[300], old_theme[64], prc[1100];
	int rc;

	if (load_theme(&t, &c, profile))
		return 1;
	if (config_home(cfg, sizeof cfg) || data_home(data, sizeof data) || state_home(st, sizeof st)) {
		fputs("rros-theme: no se pudo determinar ~/.config\n", stderr);
		rros_conf_free(&c);
		return 1;
	}
	t.soft_render = soft_render();
	rros_theme_palette(&t, &p);
	snprintf(prc, sizeof prc, "%s/plasmarc", cfg);
	rros_ini_get(prc, "Theme", "name", old_theme, sizeof old_theme);
	/* primero la configuración (rápido) y después el fondo, que la primera vez tarda medio segundo */
	rc = render_all(cfg, data, &t, &c);
	ensure_wallpaper(&t, &p, wp, sizeof wp);
	plasma_colors(&t, &p, &c, data, cfg, quiet);
	plasma_layout(&t, &p, &c, force_layout);
	plasma_reload(&t, old_theme, wp, st);
	if (rc == 0 && !quiet)
		printf("tema aplicado: %s (%s, acento %s, dock %s%s)\n", t.theme, t.dark ? "oscuro" : "claro", p.accent,
		       t.dock_3d ? "3D" : "2D", t.soft_render ? ", sin aceleración gráfica: efectos apagados" : "");
	rros_conf_free(&c);
	return rc;
}

static int cmd_apply(const char *profile, int quiet)
{
	return cmd_apply_ex(profile, quiet, 0);
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
		fprintf(stderr, "rros-theme: no hay ningún estilo de escritorio '%s' (rros, clasico, isla, moderno, doble, lateral)\n", name);
		return 1;
	}
	for (size_t k = 0; k < 14 && d->keys[k][0]; k++)
		if (rros_user_set(d->keys[k][0], d->keys[k][1], 0, err, sizeof err)) {
			fprintf(stderr, "rros-theme: %s\n", err);
			return 1;
		}
	printf("escritorio: %s\n", d->label);
	return cmd_apply_ex(profile, 1, 1);
}

static int cmd_wallpaper(const char *profile, int argc, char **argv, int i)
{
	static const char *styles[] = { "aurora", "ondas", "degradado", "circulos", "petalos", "dunas", "malla",
					"cristal", "topografico", "seda", "liso" };
	const size_t ns = sizeof styles / sizeof *styles - 1;   /* generados (sin «liso») */
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

		while (k < ns && strcmp(styles[k], t.wallpaper_style))
			k++;
		if (rros_user_set("ui.wallpaper_style", styles[(k + 1) % ns], 0, err, sizeof err) ||
		    (t.wallpaper[0] && rros_user_unset("ui.wallpaper", 0, err, sizeof err))) {
			fprintf(stderr, "rros-theme: %s\n", err);
			return 1;
		}
		printf("fondo: %s\n", styles[(k + 1) % ns]);
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

		while (k < ns && strcmp(styles[k], style))
			k++;
		if (k == ns) {
			fprintf(stderr, "rros-theme: estilo de fondo desconocido: %s (aurora, ondas, degradado, circulos, petalos, dunas, malla, cristal, topografico, seda)\n", style);
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
		const char *name = argv[i], *base = "lirio";
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
		char path[1100];
		struct rros_palette p;
		FILE *f;

		for (; i < argc; i++) {
			if (!strcmp(argv[i], "--out") && i + 1 < argc)
				out = argv[++i];
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
		{
			const char *sr = getenv("RROS_SOFT_RENDER");

			t.soft_render = sr && !strcmp(sr, "1");
		}
		i = render_all(out, out, &t, &conf);
		rros_theme_palette(&t, &p);
		snprintf(path, sizeof path, "%s/plasma/layout.js", out);
		if (rros_mkdir_parents(path) == 0 && (f = fopen(path, "w"))) {
			rros_render_plasma_layout(&t, &p, &conf, f);
			fclose(f);
		} else {
			i = 1;
		}
		snprintf(path, sizeof path, "%s/kde/RROS.colors", out);
		if (rros_mkdir_parents(path) == 0 && (f = fopen(path, "w"))) {
			rros_render_kde_colors(&t, &p, &conf, f);
			fclose(f);
		} else {
			i = 1;
		}
		snprintf(path, sizeof path, "%s/aurorae/RROS", out);
		i |= rros_write_aurorae(path, &t, &p) != 0;
		rros_conf_free(&conf);
		return i;
	}
	usage();
	return 2;
}
