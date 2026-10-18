/* rros-apps: tienda de aplicaciones de RROS (pacman o apt, según la base, y Flatpak). Nunca usa una shell.
 * RROS_PKG=pacman|apt fuerza el gestor de paquetes (pruebas). */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "rros_conf.h"
#include "rros_profile.h"

#define MAXAPPS 256
#define MAXPKGS 8
#define FLATHUB_URL "https://dl.flathub.org/repo/flathub.flatpakrepo"

struct app {
	char id[40];
	const char *name, *category, *desc, *apt, *pacman, *flatpak, *shipped, *cmd;
};

enum { PKG_APT, PKG_PACMAN };

static struct app apps[MAXAPPS];
static size_t napps;
static struct rros_conf cat;
static int dry_run;
static int backend = PKG_APT;
static const char *const backend_name[] = { "apt", "pacman" };

/* ---- validación ---- */

static int all_in(const char *s, const char *extra)
{
	if (!*s)
		return 0;
	for (; *s; s++)
		if (!isalnum((unsigned char)*s) && !strchr(extra, *s))
			return 0;
	return 1;
}

static int id_ok(const char *s)
{
	for (const char *p = s; *p; p++)
		if (isupper((unsigned char)*p))
			return 0;
	return strlen(s) < 40 && all_in(s, "-");
}

/* ---- catálogo ---- */

static const char *catalog_path(char *buf, size_t n)
{
	const char *env = getenv("RROS_APPS_CATALOG");

	if (env && *env)
		return env;
	if (access("/usr/share/rros/apps/catalog.toml", R_OK) == 0)
		return "/usr/share/rros/apps/catalog.toml";
	if (access("apps/catalog.toml", R_OK) == 0)
		return "apps/catalog.toml";
	snprintf(buf, n, "../apps/catalog.toml");
	return buf;
}

static struct app *find_app(const char *id)
{
	for (size_t i = 0; i < napps; i++)
		if (!strcmp(apps[i].id, id))
			return &apps[i];
	return NULL;
}

static int load_catalog(void)
{
	char fb[64];
	const char *path = catalog_path(fb, sizeof fb);
	int line = 0, rc;

	rros_conf_init(&cat);
	rc = rros_conf_load(&cat, path, &line);
	if (rc) {
		fprintf(stderr, "rros-apps: no se pudo leer el catálogo %s%s\n", path, rc == -2 ? " (error de sintaxis)" : "");
		return -1;
	}
	for (size_t i = 0; i < cat.n; i++) {
		const struct rros_kv *kv = &cat.items[i];
		const char *last, *field;
		char id[48];
		struct app *a;
		size_t il;

		if (strncmp(kv->key, "app.", 4))
			continue;
		last = strrchr(kv->key, '.');
		if (last == kv->key + 3)
			continue;
		il = (size_t)(last - (kv->key + 4));
		if (il == 0 || il >= sizeof id)
			continue;
		memcpy(id, kv->key + 4, il);
		id[il] = '\0';
		field = last + 1;
		if (!id_ok(id))
			continue;
		a = find_app(id);
		if (!a) {
			if (napps >= MAXAPPS)
				continue;
			a = &apps[napps++];
			memset(a, 0, sizeof *a);
			snprintf(a->id, sizeof a->id, "%s", id);
		}
		if (!strcmp(field, "name")) a->name = kv->val;
		else if (!strcmp(field, "category")) a->category = kv->val;
		else if (!strcmp(field, "desc")) a->desc = kv->val;
		else if (!strcmp(field, "apt")) a->apt = all_in(kv->val, ".+-") ? kv->val : NULL;
		else if (!strcmp(field, "pacman")) a->pacman = all_in(kv->val, ".+-_,") ? kv->val : NULL;
		else if (!strcmp(field, "flatpak")) a->flatpak = all_in(kv->val, "._-") ? kv->val : NULL;
		else if (!strcmp(field, "shipped")) a->shipped = kv->val;
		else if (!strcmp(field, "cmd")) a->cmd = all_in(kv->val, "-_") ? kv->val : NULL;
	}
	for (size_t i = 0; i < napps; i++) {
		if (!apps[i].name)
			apps[i].name = apps[i].id;
		if (!apps[i].category)
			apps[i].category = "utility";
		if (!apps[i].desc)
			apps[i].desc = "";
	}
	return 0;
}

/* ---- procesos ---- */

static void show_cmd(char *const argv[])
{
	fputs("$", stdout);
	for (int i = 0; argv[i]; i++)
		printf(" %s", argv[i]);
	putchar('\n');
	fflush(stdout);
}

/* Devuelve el código de salida; con capture!=NULL guarda la salida estándar. */
static int run(char *const argv[], char *capture, size_t cn, int quiet)
{
	int pfd[2] = { -1, -1 }, st;
	pid_t pid;

	if (capture && pipe(pfd) != 0)
		return -1;
	fflush(stdout);
	pid = fork();
	if (pid < 0)
		return -1;
	if (pid == 0) {
		if (capture) {
			dup2(pfd[1], STDOUT_FILENO);
			close(pfd[0]);
			close(pfd[1]);
		}
		if (quiet || capture) {
			if (!freopen("/dev/null", "w", stderr))
				_exit(127);
		}
		setenv("DEBIAN_FRONTEND", "noninteractive", 1);
		execvp(argv[0], argv);
		_exit(127);
	}
	if (capture) {
		ssize_t k;
		size_t used = 0;

		close(pfd[1]);
		while (used + 1 < cn && (k = read(pfd[0], capture + used, cn - 1 - used)) > 0)
			used += (size_t)k;
		capture[used] = '\0';
		close(pfd[0]);
	}
	if (waitpid(pid, &st, 0) < 0)
		return -1;
	return WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}

static int step(char *const argv[])
{
	show_cmd(argv);
	if (dry_run)
		return 0;
	return run(argv, NULL, 0, 0);
}

static int have_cmd(const char *c)
{
	const char *path = getenv("PATH");
	char buf[700];

	while (path && *path) {
		const char *colon = strchr(path, ':');
		size_t n = colon ? (size_t)(colon - path) : strlen(path);

		snprintf(buf, sizeof buf, "%.*s/%s", (int)n, path, c);
		if (access(buf, X_OK) == 0)
			return 1;
		if (!colon)
			break;
		path = colon + 1;
	}
	return 0;
}

/* Prefijo para ejecutar con privilegios: nada si ya somos root, si no pkexec o sudo. */
static const char *priv(void)
{
	if (geteuid() == 0)
		return NULL;
	if (have_cmd("pkexec"))
		return "pkexec";
	if (have_cmd("sudo"))
		return "sudo";
	return NULL;
}

/* ---- gestor de paquetes ---- */

static int detect_backend(void)
{
	const char *e = getenv("RROS_PKG");

	if (e && !strcmp(e, "pacman"))
		return PKG_PACMAN;
	if (e && !strcmp(e, "apt"))
		return PKG_APT;
	if (access("/etc/arch-release", F_OK) == 0 || (have_cmd("pacman") && !have_cmd("apt-get")))
		return PKG_PACMAN;
	return PKG_APT;
}

/* los paquetes de la app en este sistema ("a,b,c"), o NULL si no tiene */
static const char *pkgs_of(const struct app *a)
{
	return backend == PKG_PACMAN ? a->pacman : a->apt;
}

/* parte "a,b,c" en argv[n...]; devuelve el nuevo n */
static int split_pkgs(const char *list, char *buf, size_t bn, char **argv, int n)
{
	char *tok, *save = NULL;
	int k = 0;

	snprintf(buf, bn, "%s", list);
	for (tok = strtok_r(buf, ",", &save); tok && k < MAXPKGS; tok = strtok_r(NULL, ",", &save), k++)
		argv[n++] = tok;
	return n;
}

/* ---- estado ---- */

/* instalada si lo está el primer paquete de la lista (el programa; los demás son idioma o extras) */
static int pkg_installed(const char *list)
{
	char first[64], out[64] = "";
	size_t l = strcspn(list, ",");

	snprintf(first, sizeof first, "%.*s", (int)l, list);
	if (backend == PKG_PACMAN) {
		char *argv[] = { "pacman", "-Q", "--", first, NULL };

		return run(argv, out, sizeof out, 1) == 0;
	} else {
		char *argv[] = { "dpkg-query", "-W", "-f=${db:Status-Status}", first, NULL };

		if (run(argv, out, sizeof out, 1) != 0)
			return 0;
		return !strcmp(out, "installed");
	}
}

static int flatpak_installed(const char *id)
{
	char *argv[] = { "flatpak", "info", (char *)id, NULL };
	char out[8];

	return run(argv, out, sizeof out, 1) == 0;
}

static int is_installed(const struct app *a)
{
	return (pkgs_of(a) && pkg_installed(pkgs_of(a))) || (a->flatpak && flatpak_installed(a->flatpak));
}

static int shipped_in(const struct app *a, const char *profile)
{
	char buf[64], *tok, *save = NULL;

	if (!a->shipped)
		return 0;
	snprintf(buf, sizeof buf, "%s", a->shipped);
	for (tok = strtok_r(buf, ",", &save); tok; tok = strtok_r(NULL, ",", &save))
		if (!strcmp(tok, profile))
			return 1;
	return 0;
}

/* ---- comandos ---- */

static void json_str(const char *s)
{
	putchar('"');
	for (; *s; s++) {
		if (*s == '"' || *s == '\\')
			printf("\\%c", *s);
		else if ((unsigned char)*s < 0x20)
			printf("\\u%04x", (unsigned char)*s);
		else
			putchar(*s);
	}
	putchar('"');
}

static int match_text(const struct app *a, const char *q)
{
	char hay[600], needle[100];
	size_t i;

	snprintf(hay, sizeof hay, "%s %s %s", a->id, a->name, a->desc);
	snprintf(needle, sizeof needle, "%s", q);
	for (i = 0; hay[i]; i++)
		hay[i] = (char)tolower((unsigned char)hay[i]);
	for (i = 0; needle[i]; i++)
		needle[i] = (char)tolower((unsigned char)needle[i]);
	return strstr(hay, needle) != NULL;
}

static int cmd_list(int argc, char **argv, const char *query)
{
	const char *only_cat = NULL;
	int only_inst = 0, json = 0, first = 1;
	char profile[64];

	for (int i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "--category") && i + 1 < argc)
			only_cat = argv[++i];
		else if (!strcmp(argv[i], "--installed"))
			only_inst = 1;
		else if (!strcmp(argv[i], "--json"))
			json = 1;
		else
			return 2;
	}
	rros_read_active_profile(profile, sizeof profile);
	if (json)
		putchar('[');
	for (size_t i = 0; i < napps; i++) {
		const struct app *a = &apps[i];
		int inst;

		if (only_cat && strcmp(a->category, only_cat))
			continue;
		if (query && !match_text(a, query))
			continue;
		inst = is_installed(a);
		if (only_inst && !inst && !shipped_in(a, profile))
			continue;
		if (json) {
			printf("%s{\"id\":", first ? "" : ",");
			json_str(a->id);
			printf(",\"name\":"); json_str(a->name);
			printf(",\"category\":"); json_str(a->category);
			printf(",\"desc\":"); json_str(a->desc);
			printf(",\"apt\":"); json_str(a->apt ? a->apt : "");
			printf(",\"pacman\":"); json_str(a->pacman ? a->pacman : "");
			printf(",\"pkg\":"); json_str(pkgs_of(a) ? pkgs_of(a) : "");
			printf(",\"pkg_manager\":\"%s\"", backend_name[backend]);
			printf(",\"flatpak\":"); json_str(a->flatpak ? a->flatpak : "");
			printf(",\"cmd\":"); json_str(a->cmd ? a->cmd : "");
			printf(",\"shipped\":%s,\"installed\":%s}", shipped_in(a, profile) ? "true" : "false", inst ? "true" : "false");
		} else {
			printf("%-14s %-26.26s %-9s %s\n", a->id, a->name, a->category,
			       inst ? "instalada" : shipped_in(a, profile) ? "incluida" : "-");
		}
		first = 0;
	}
	if (json)
		puts("]");
	else if (first)
		puts("(nada que mostrar)");
	return 0;
}

static int cmd_categories(void)
{
	static const struct { const char *id, *label; } cats[] = {
		{ "internet", "Internet" }, { "office", "Oficina" }, { "media", "Multimedia" }, { "graphics", "Gráficos" },
		{ "dev", "Desarrollo" }, { "system", "Sistema" }, { "games", "Juegos" }, { "utility", "Utilidades" },
	};

	for (size_t c = 0; c < sizeof cats / sizeof *cats; c++) {
		int n = 0;

		for (size_t i = 0; i < napps; i++)
			n += !strcmp(apps[i].category, cats[c].id);
		printf("%-10s %-12s %d\n", cats[c].id, cats[c].label, n);
	}
	return 0;
}

static int cmd_info(const struct app *a)
{
	char profile[64];

	rros_read_active_profile(profile, sizeof profile);
	printf("%s (%s)\n  %s\n", a->name, a->id, a->desc);
	printf("  categoría: %s\n", a->category);
	if (pkgs_of(a))
		printf("  %-10s %s\n", backend == PKG_PACMAN ? "pacman:" : "apt:", pkgs_of(a));
	if (a->flatpak)
		printf("  flatpak:   %s\n", a->flatpak);
	if (a->cmd)
		printf("  se lanza:  %s\n", a->cmd);
	printf("  estado:    %s\n", is_installed(a) ? "instalada" : shipped_in(a, profile) ? "incluida en tu perfil (pero no instalada)" : "no instalada");
	return 0;
}

static int flathub_setup(void)
{
	char *argv[] = { "flatpak", "remote-add", "--user", "--if-not-exists", "flathub", FLATHUB_URL, NULL };

	if (!have_cmd("flatpak") && !dry_run) {
		fputs("rros-apps: flatpak no está instalado (instálalo con el gestor de paquetes del sistema)\n", stderr);
		return 1;
	}
	return step(argv) == 0 ? 0 : 1;
}

/* verb: install, remove o upgrade (todo el sistema); pkgs: "a,b,c" o NULL.
 * En Arch se instala siempre con -Syu: actualizar solo una parte del sistema rompe dependencias. En el sistema
 * live no: todo vive en la memoria y se pierde al apagar, y actualizar el sistema entero no cabría. */
static int live_system(void)
{
	char buf[2048] = "";
	FILE *f = fopen("/proc/cmdline", "r");
	const char *e = getenv("RROS_LIVE");

	if (e)
		return !strcmp(e, "1");
	if (f) {
		if (!fgets(buf, sizeof buf, f))
			buf[0] = '\0';
		fclose(f);
	}
	return strstr(buf, "archisobasedir=") != NULL;
}

static int pkg_step(const char *verb, const char *pkgs)
{
	const char *p = priv();
	char *argv[8 + MAXPKGS], buf[256];
	int n = 0;

	if (p)
		argv[n++] = (char *)p;
	if (backend == PKG_PACMAN) {
		argv[n++] = "pacman";
		if (!strcmp(verb, "remove")) {
			argv[n++] = "-Rns";
		} else {
			argv[n++] = pkgs && live_system() ? "-Sy" : "-Syu";
			if (pkgs)
				argv[n++] = "--needed";
		}
		argv[n++] = "--noconfirm";
	} else {
		if (strcmp(verb, "remove")) {   /* install y upgrade: antes, la lista de paquetes */
			char *upd[4];
			int u = 0;

			if (p)
				upd[u++] = (char *)p;
			upd[u++] = "apt-get";
			upd[u++] = "update";
			upd[u] = NULL;
			if (step(upd) != 0)
				return 1;
		}
		argv[n++] = "apt-get";
		argv[n++] = (char *)verb;
		argv[n++] = "-y";
	}
	if (pkgs)
		n = split_pkgs(pkgs, buf, sizeof buf, argv, n);
	argv[n] = NULL;
	return step(argv);
}

static int cmd_install(const struct app *a, int force_flatpak)
{
	const char *pkgs = pkgs_of(a);
	int use_flatpak = (force_flatpak && a->flatpak) || !pkgs;

	if (use_flatpak && !a->flatpak) {
		fprintf(stderr, "rros-apps: '%s' no tiene origen de instalación\n", a->id);
		return 1;
	}
	if (force_flatpak && !a->flatpak) {
		fprintf(stderr, "rros-apps: '%s' no está en Flathub; se usará %s\n", a->id, backend_name[backend]);
		use_flatpak = 0;
	}
	if (!dry_run && is_installed(a)) {
		printf("%s ya está instalada\n", a->name);
		return 0;
	}
	printf("Instalando %s con %s...\n", a->name, use_flatpak ? "Flatpak (Flathub)" : backend_name[backend]);
	if (use_flatpak) {
		char *inst[] = { "flatpak", "install", "--user", "-y", "--noninteractive", "flathub", (char *)a->flatpak, NULL };

		if (flathub_setup() || step(inst) != 0) {
			fprintf(stderr, "rros-apps: no se pudo instalar %s\n", a->name);
			return 1;
		}
	} else {
		if (pkg_step("install", pkgs) != 0) {
			fprintf(stderr, "rros-apps: no se pudo instalar %s\n", a->name);
			return 1;
		}
	}
	printf("Listo: %s%s\n", a->name, a->cmd ? "; ábrela desde el lanzador (Mod+D)" : "");
	return 0;
}

static int cmd_remove(const struct app *a)
{
	int done = 0;

	if (a->flatpak && (dry_run || flatpak_installed(a->flatpak))) {
		char *argv[] = { "flatpak", "uninstall", "--user", "-y", (char *)a->flatpak, NULL };

		if (step(argv) != 0)
			return 1;
		done = 1;
	}
	if (pkgs_of(a) && (dry_run || pkg_installed(pkgs_of(a)))) {
		if (pkg_step("remove", pkgs_of(a)) != 0)
			return 1;
		done = 1;
	}
	if (!done)
		printf("%s no está instalada\n", a->name);
	return 0;
}

static int cmd_update(void)
{
	char *fp[] = { "flatpak", "update", "--user", "-y", NULL };
	int rc = 0;

	if (pkg_step("upgrade", NULL) != 0)
		rc = 1;
	if ((dry_run || have_cmd("flatpak")) && step(fp) != 0)
		rc = 1;
	return rc;
}

static void usage(FILE *out)
{
	fputs("uso: rros-apps COMANDO\n"
	      "  list [--category C] [--installed] [--json]   catálogo y estado\n"
	      "  categories                                   categorías\n"
	      "  search TEXTO                                 buscar por nombre o descripción\n"
	      "  info ID                                      detalles de una app\n"
	      "  install ID [--flatpak] [--dry-run]           instalar (pacman o apt; Flatpak si no hay paquete)\n"
	      "  remove ID [--dry-run]                        quitar\n"
	      "  update [--dry-run]                           actualizar sistema y aplicaciones\n"
	      "  setup [--dry-run]                            añadir Flathub (se hace solo al instalar)\n"
	      "\n"
	      "Ejemplo: rros apps install firefox\n", out);
}

int main(int argc, char **argv)
{
	const char *cmd;
	int force_flatpak = 0, rc = 0;
	const char *id = NULL;

	if (argc < 2 || !strcmp(argv[1], "help") || !strcmp(argv[1], "--help")) {
		usage(argc < 2 ? stderr : stdout);
		return argc < 2 ? 2 : 0;
	}
	cmd = argv[1];
	backend = detect_backend();
	if (load_catalog() != 0)
		return 1;
	if (!strcmp(cmd, "list"))
		return cmd_list(argc - 2, argv + 2, NULL) == 2 ? (usage(stderr), 2) : 0;
	if (!strcmp(cmd, "categories"))
		return cmd_categories();
	if (!strcmp(cmd, "search") && argc == 3)
		return cmd_list(0, argv, argv[2]);
	if (!strcmp(cmd, "update") || !strcmp(cmd, "setup")) {
		for (int i = 2; i < argc; i++) {
			if (!strcmp(argv[i], "--dry-run"))
				dry_run = 1;
			else {
				usage(stderr);
				return 2;
			}
		}
		return !strcmp(cmd, "update") ? cmd_update() : flathub_setup();
	}
	if ((!strcmp(cmd, "info") || !strcmp(cmd, "install") || !strcmp(cmd, "remove")) && argc >= 3) {
		struct app *a;

		id = argv[2];
		for (int i = 3; i < argc; i++) {
			if (!strcmp(argv[i], "--dry-run"))
				dry_run = 1;
			else if (!strcmp(argv[i], "--flatpak"))
				force_flatpak = 1;
			else {
				usage(stderr);
				return 2;
			}
		}
		a = find_app(id);
		if (!a) {
			fprintf(stderr, "rros-apps: no hay ninguna app '%s' (prueba `rros apps search %s`)\n", id, id);
			return 1;
		}
		if (!strcmp(cmd, "info"))
			rc = cmd_info(a);
		else if (!strcmp(cmd, "install"))
			rc = cmd_install(a, force_flatpak);
		else
			rc = cmd_remove(a);
		return rc;
	}
	usage(stderr);
	return 2;
}
