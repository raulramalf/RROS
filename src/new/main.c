/* rros-new: crea archivos y proyectos a partir de plantillas.
 *   rros-new                                  asistente (elige plantilla, nombre y carpeta)
 *   rros-new lista                            plantillas disponibles (de Lily y tuyas)
 *   rros-new PLANTILLA NOMBRE [--dir CARPETA] [--force] [--abrir]
 *   rros-new exportar CARPETA                 copia las plantillas a una carpeta (p. ej. ~/Plantillas)
 * Plantillas de Lily: $RROS_TEMPLATES_DIR o /usr/share/rros/templates (plantillas.toml).
 * Plantillas tuyas: cualquier archivo de tu carpeta de plantillas (XDG_TEMPLATES_DIR, ~/Plantillas). */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "rros_conf.h"
#include "rros_profile.h"

#define MAXT 64

struct tpl {
	char id[48], desc[160], file[160], dir[160], dest[200];
	int exec, user;      /* user: plantilla de ~/Plantillas */
};

static struct tpl tpls[MAXT];
static int ntpl;
static char sysdir[512], userdir[512];

/* ---- rutas ---- */

static const char *home(void)
{
	const char *h = getenv("HOME");
	struct passwd *pw;

	if (h && *h)
		return h;
	pw = getpwuid(getuid());
	return pw ? pw->pw_dir : "/";
}

static void expand(const char *in, char *out, size_t n)
{
	if (in[0] == '~' && (in[1] == '/' || !in[1]))
		snprintf(out, n, "%s%s", home(), in + 1);
	else if (!strncmp(in, "$HOME", 5))
		snprintf(out, n, "%s%s", home(), in + 5);
	else
		snprintf(out, n, "%s", in);
}

/* carpeta de plantillas del usuario: XDG_TEMPLATES_DIR de user-dirs.dirs, o ~/Plantillas */
static void find_userdir(void)
{
	char path[600], line[600];
	const char *xdg = getenv("XDG_CONFIG_HOME");
	FILE *f;

	userdir[0] = '\0';
	if (xdg && *xdg)
		snprintf(path, sizeof path, "%s/user-dirs.dirs", xdg);
	else
		snprintf(path, sizeof path, "%s/.config/user-dirs.dirs", home());
	f = fopen(path, "r");
	while (f && fgets(line, sizeof line, f)) {
		char *q1, *q2;

		if (strncmp(line, "XDG_TEMPLATES_DIR=", 18) || !(q1 = strchr(line, '"')) || !(q2 = strrchr(line, '"')) || q2 <= q1)
			continue;
		*q2 = '\0';
		expand(q1 + 1, userdir, sizeof userdir);
	}
	if (f)
		fclose(f);
	if (!userdir[0] || !strcmp(userdir, home()))
		snprintf(userdir, sizeof userdir, "%s/Plantillas", home());
}

/* ---- catálogo ---- */

static void load(void)
{
	char path[600];
	struct rros_conf c;
	int errline = 0;
	const char *env = getenv("RROS_TEMPLATES_DIR");

	snprintf(sysdir, sizeof sysdir, "%s", env && *env ? env : "/usr/share/rros/templates");
	snprintf(path, sizeof path, "%s/plantillas.toml", sysdir);
	rros_conf_init(&c);
	if (rros_conf_load(&c, path, &errline) == 0) {
		for (size_t i = 0; i < c.n && ntpl < MAXT; i++) {
			const char *k = c.items[i].key, *dot = strrchr(k, '.');
			char id[48], key[96];
			struct tpl *t;
			const char *v;

			if (!dot || strcmp(dot, ".desc") || dot - k >= (long)sizeof id)
				continue;
			snprintf(id, sizeof id, "%.*s", (int)(dot - k), k);
			t = &tpls[ntpl++];
			memset(t, 0, sizeof *t);
			snprintf(t->id, sizeof t->id, "%s", id);
			snprintf(t->desc, sizeof t->desc, "%s", c.items[i].val);
			snprintf(key, sizeof key, "%s.file", id);
			if ((v = rros_conf_get(&c, key)))
				snprintf(t->file, sizeof t->file, "%s", v);
			snprintf(key, sizeof key, "%s.dir", id);
			if ((v = rros_conf_get(&c, key)))
				snprintf(t->dir, sizeof t->dir, "%s", v);
			snprintf(key, sizeof key, "%s.destino", id);
			if ((v = rros_conf_get(&c, key)))
				snprintf(t->dest, sizeof t->dest, "%s", v);
			snprintf(key, sizeof key, "%s.exec", id);
			t->exec = rros_conf_get_bool(&c, key, 0);
			if (!t->file[0] && !t->dir[0])
				ntpl--;
		}
	} else {
		fprintf(stderr, "rros-new: aviso: no se pudo leer %s\n", path);
	}
	rros_conf_free(&c);

	/* plantillas del usuario (las que ya son de Lily, copiadas por exportar, no se repiten) */
	find_userdir();
	{
		DIR *d = opendir(userdir);
		struct dirent *e;

		while (d && (e = readdir(d)) && ntpl < MAXT) {
			char p[1100];
			struct stat st;
			int dup = 0;
			struct tpl *t;

			if (e->d_name[0] == '.' || strlen(e->d_name) >= sizeof tpls[0].file)
				continue;
			snprintf(p, sizeof p, "%s/%s", userdir, e->d_name);
			if (lstat(p, &st) || !S_ISREG(st.st_mode))
				continue;
			for (int i = 0; i < ntpl; i++)
				if (!strcmp(tpls[i].file, e->d_name))
					dup = 1;
			if (dup)
				continue;
			t = &tpls[ntpl++];
			memset(t, 0, sizeof *t);
			snprintf(t->id, sizeof t->id, "%.47s", e->d_name);
			snprintf(t->file, sizeof t->file, "%s", e->d_name);
			snprintf(t->desc, sizeof t->desc, "plantilla tuya (%s)", "Plantillas");
			t->exec = (st.st_mode & 0100) != 0;
			t->user = 1;
		}
		if (d)
			closedir(d);
	}
}

static const struct tpl *find(const char *id)
{
	for (int i = 0; i < ntpl; i++)
		if (!strcmp(tpls[i].id, id))
			return &tpls[i];
	/* una plantilla tuya también se encuentra por su nombre sin extensión */
	for (int i = 0; i < ntpl; i++) {
		const char *dot = strrchr(tpls[i].file, '.');

		if (tpls[i].user && dot && (size_t)(dot - tpls[i].file) == strlen(id) && !strncmp(tpls[i].file, id, strlen(id)))
			return &tpls[i];
	}
	return NULL;
}

/* ---- marcadores ---- */

struct vars { char nombre[256], archivo[256], fecha[16], anio[8], usuario[64]; };

static void set_vars(struct vars *v, const char *name, const char *file)
{
	time_t now = time(NULL);
	struct tm tm;
	struct passwd *pw = getpwuid(getuid());

	localtime_r(&now, &tm);
	snprintf(v->nombre, sizeof v->nombre, "%s", name);
	snprintf(v->archivo, sizeof v->archivo, "%s", file);
	strftime(v->fecha, sizeof v->fecha, "%Y-%m-%d", &tm);
	strftime(v->anio, sizeof v->anio, "%Y", &tm);
	snprintf(v->usuario, sizeof v->usuario, "%s", pw ? pw->pw_name : getenv("USER") ? getenv("USER") : "usuario");
}

static void substitute(FILE *out, const char *text, const struct vars *v)
{
	static const char *names[] = { "{{NOMBRE}}", "{{ARCHIVO}}", "{{FECHA}}", "{{AÑO}}", "{{USUARIO}}" };
	const char *vals[] = { v->nombre, v->archivo, v->fecha, v->anio, v->usuario };

	while (*text) {
		int hit = 0;

		if (text[0] == '{' && text[1] == '{')
			for (size_t i = 0; i < sizeof names / sizeof *names && !hit; i++)
				if (!strncmp(text, names[i], strlen(names[i]))) {
					fputs(vals[i], out);
					text += strlen(names[i]);
					hit = 1;
				}
		if (!hit)
			fputc(*text++, out);
	}
}

static char *slurp(const char *path)
{
	FILE *f = fopen(path, "rb");
	char *buf;
	long n;

	if (!f)
		return NULL;
	fseek(f, 0, SEEK_END);
	n = ftell(f);
	rewind(f);
	if (n < 0 || n > 4 * 1024 * 1024 || !(buf = malloc((size_t)n + 1))) {
		fclose(f);
		return NULL;
	}
	buf[fread(buf, 1, (size_t)n, f)] = '\0';
	fclose(f);
	return buf;
}

/* escribe `src` con los marcadores sustituidos en `dst`; sin --force no pisa nada */
static int render_file(const char *src, const char *dst, const struct vars *v, int exec, int force)
{
	char *text = slurp(src);
	int fd;
	FILE *out;

	if (!text) {
		fprintf(stderr, "rros-new: no se pudo leer la plantilla %s\n", src);
		return -1;
	}
	fd = open(dst, O_WRONLY | O_CREAT | (force ? O_TRUNC : O_EXCL), exec ? 0777 : 0666);
	if (fd < 0) {
		fprintf(stderr, "rros-new: %s: %s\n", dst, errno == EEXIST ? "ya existe (usa --force para reemplazarlo)" :
			errno == ENOENT ? "la carpeta no existe" : errno == EACCES ? "sin permiso para escribir ahí" : strerror(errno));
		free(text);
		return -1;
	}
	if (force && exec)
		fchmod(fd, 0755);
	out = fdopen(fd, "w");
	if (!out) {
		close(fd);
		free(text);
		return -1;
	}
	substitute(out, text, v);
	free(text);
	return fclose(out) == 0 ? 0 : -1;
}

static int copy_tree(const char *src, const char *dst, const struct vars *v)
{
	DIR *d = opendir(src);
	struct dirent *e;
	int rc = 0;

	if (!d)
		return -1;
	if (mkdir(dst, 0777) != 0) {
		fprintf(stderr, "rros-new: %s: %s\n", dst, errno == EEXIST ? "ya existe" : errno == ENOENT ? "la carpeta no existe" : strerror(errno));
		closedir(d);
		return -1;
	}
	while (rc == 0 && (e = readdir(d))) {
		char s[1100], t[1100];
		struct stat st;

		if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
			continue;
		snprintf(s, sizeof s, "%s/%s", src, e->d_name);
		snprintf(t, sizeof t, "%s/%s", dst, e->d_name);
		if (lstat(s, &st))
			continue;
		if (S_ISDIR(st.st_mode))
			rc = copy_tree(s, t, v);
		else if (S_ISREG(st.st_mode))
			rc = render_file(s, t, v, (st.st_mode & 0111) != 0, 0);
	}
	closedir(d);
	return rc;
}

static int valid_name(const char *n)
{
	if (!*n || !strcmp(n, ".") || !strcmp(n, "..") || n[0] == '-' || strlen(n) > 200)
		return 0;
	for (; *n; n++)
		if ((unsigned char)*n < 0x20)
			return 0;
	return 1;
}

static void open_path(const char *path)
{
	pid_t pid = fork();

	if (pid == 0) {
		setsid();
		if (!freopen("/dev/null", "w", stdout) || !freopen("/dev/null", "w", stderr))
			_exit(127);
		execlp("xdg-open", "xdg-open", path, (char *)NULL);
		_exit(127);
	}
}

/* crea desde la plantilla t; devuelve la ruta creada en `made` */
static int create(const struct tpl *t, const char *name, const char *dir_opt, int force, char *made, size_t mn)
{
	char dir[600], base[260], target[1100], src[1100];
	const char *ext = t->file[0] ? strrchr(t->file, '.') : NULL, *slash = strrchr(name, '/');
	struct vars v;

	if (!valid_name(name)) {
		fprintf(stderr, "rros-new: nombre no válido: '%s'\n", name);
		return 2;
	}
	if (dir_opt)
		expand(dir_opt, dir, sizeof dir);
	else if (t->dest[0])
		expand(t->dest, dir, sizeof dir);
	else
		snprintf(dir, sizeof dir, ".");
	/* sin extensión, se pone la de la plantilla: `rros-new web inicio` crea inicio.html */
	if (ext && !strchr(slash ? slash + 1 : name, '.') && ext != t->file)
		snprintf(base, sizeof base, "%s%s", name, ext);
	else
		snprintf(base, sizeof base, "%s", name);
	if (base[0] == '/')
		snprintf(target, sizeof target, "%s", base);
	else
		snprintf(target, sizeof target, "%s/%s", dir, base);
	if (t->dest[0])
		rros_mkdir_parents(target);   /* p. ej. ~/.local/share/applications puede no existir aún */
	{
		const char *fname = strrchr(target, '/') ? strrchr(target, '/') + 1 : target;
		char stem[260];
		const char *dot = strrchr(fname, '.');

		snprintf(stem, sizeof stem, "%.*s", dot && dot != fname ? (int)(dot - fname) : (int)strlen(fname), fname);
		set_vars(&v, stem, fname);
	}
	if (t->dir[0]) {
		snprintf(src, sizeof src, "%s/%s", sysdir, t->dir);
		if (copy_tree(src, target, &v))
			return 1;
	} else {
		snprintf(src, sizeof src, "%s/%s", t->user ? userdir : sysdir, t->file);
		if (render_file(src, target, &v, t->exec, force))
			return 1;
	}
	{
		char *abs = realpath(target, NULL);

		snprintf(made, mn, "%s", abs ? abs : target);
		free(abs);
	}
	return 0;
}

static void json_str(const char *s)
{
	putchar('"');
	for (; *s; s++) {
		if (*s == '"' || *s == '\\')
			printf("\\%c", *s);
		else if ((unsigned char)*s < 0x20)
			printf("\\u%04x", *s);
		else
			putchar(*s);
	}
	putchar('"');
}

static void list_json(void)
{
	char docs[600];

	snprintf(docs, sizeof docs, "%s/Documentos", home());
	if (access(docs, F_OK) != 0)
		snprintf(docs, sizeof docs, "%s", home());
	fputs("{\"default_dir\":", stdout);
	json_str(docs);
	fputs(",\"user_dir\":", stdout);
	json_str(userdir);
	fputs(",\"templates\":[", stdout);
	for (int i = 0; i < ntpl; i++) {
		const struct tpl *t = &tpls[i];
		const char *ext = t->file[0] ? strrchr(t->file, '.') : NULL;

		printf("%s{\"id\":", i ? "," : "");
		json_str(t->id);
		fputs(",\"desc\":", stdout);
		json_str(t->desc);
		printf(",\"project\":%s,\"user\":%s,\"ext\":", t->dir[0] ? "true" : "false", t->user ? "true" : "false");
		json_str(ext && ext != t->file ? ext : "");
		fputs(",\"dest\":", stdout);
		json_str(t->dest);
		putchar('}');
	}
	puts("]}");
}

static void list(void)
{
	printf("Plantillas de Lily (rros-new PLANTILLA NOMBRE):\n");
	for (int i = 0; i < ntpl; i++) {
		if (tpls[i].user)
			continue;
		printf("  %-16s %s\n", tpls[i].id, tpls[i].desc);
	}
	printf("\nTus plantillas (%s):\n", userdir);
	{
		int any = 0;

		for (int i = 0; i < ntpl; i++)
			if (tpls[i].user) {
				printf("  %s\n", tpls[i].file);
				any = 1;
			}
		if (!any)
			printf("  (ninguna: guarda ahí cualquier archivo y aparecerá en esta lista y en «Crear documento»)\n");
	}
}

static int export_to(const char *dir_in)
{
	char dir[600];
	struct vars v;
	int made = 0, rc = 0;

	expand(dir_in, dir, sizeof dir);
	if (mkdir(dir, 0777) != 0 && errno != EEXIST) {
		fprintf(stderr, "rros-new: %s: %s\n", dir, strerror(errno));
		return 1;
	}
	for (int i = 0; i < ntpl; i++) {
		const struct tpl *t = &tpls[i];
		char src[1100], dst[1100];

		if (t->user || !t->file[0])
			continue;
		snprintf(src, sizeof src, "%s/%s", sysdir, t->file);
		snprintf(dst, sizeof dst, "%s/%s", dir, t->file);
		if (access(dst, F_OK) == 0)
			continue;   /* no se pisan las que el usuario haya cambiado */
		set_vars(&v, "Sin título", "este-archivo");
		v.fecha[0] = '\0';
		if (render_file(src, dst, &v, t->exec, 0) == 0)
			made++;
		else
			rc = 1;
	}
	printf("%d plantilla(s) copiadas a %s\n", made, dir);
	return rc;
}

static void readline_tty(const char *prompt, char *buf, size_t n, const char *def)
{
	printf("%s%s%s%s: ", prompt, def ? " [" : "", def ? def : "", def ? "]" : "");
	fflush(stdout);
	if (!fgets(buf, (int)n, stdin))
		buf[0] = '\0';
	buf[strcspn(buf, "\n")] = '\0';
	if (!buf[0] && def)
		snprintf(buf, n, "%s", def);
}

static int wizard(void)
{
	char in[300], name[260], dir[600], made[1100], docs[600];
	int choice, n = 0, idx[MAXT];

	puts("Crear un archivo nuevo\n");
	for (int i = 0; i < ntpl; i++) {
		idx[n++] = i;
		printf("  %2d) %-16s %s\n", n, tpls[i].user ? tpls[i].file : tpls[i].id, tpls[i].desc);
	}
	readline_tty("\nNúmero de plantilla", in, sizeof in, NULL);
	choice = atoi(in);
	if (choice < 1 || choice > n) {
		fprintf(stderr, "rros-new: opción no válida\n");
		return 2;
	}
	readline_tty("Nombre", name, sizeof name, "sin-titulo");
	snprintf(docs, sizeof docs, "%s/Documentos", home());
	if (tpls[idx[choice - 1]].dest[0])
		expand(tpls[idx[choice - 1]].dest, docs, sizeof docs);
	else if (access(docs, F_OK) != 0)
		snprintf(docs, sizeof docs, "%s", home());
	readline_tty("Carpeta", dir, sizeof dir, docs);
	if (create(&tpls[idx[choice - 1]], name, dir, 0, made, sizeof made))
		return 1;
	printf("Creado: %s\n", made);
	readline_tty("¿Abrirlo ahora? (s/n)", in, sizeof in, "s");
	if (in[0] == 's' || in[0] == 'S')
		open_path(made);
	return 0;
}

static void usage(void)
{
	fputs("uso: rros-new [PLANTILLA NOMBRE [--dir CARPETA] [--force] [--abrir]]\n"
	      "  (sin argumentos)       asistente\n"
	      "  lista                  plantillas disponibles\n"
	      "  exportar CARPETA       copia las plantillas (para «Crear documento» del gestor de archivos)\n"
	      "ejemplos:\n"
	      "  rros-new bash copia-seguridad          crea copia-seguridad.sh, ya ejecutable\n"
	      "  rros-new web index --dir ~/web         crea ~/web/index.html\n"
	      "  rros-new proyecto-python mi-app        crea la carpeta mi-app con su main.py\n"
	      "  rros-new lanzador \"Mi radio\"           crea un acceso directo en el menú de aplicaciones\n", stderr);
}

int main(int argc, char **argv)
{
	const char *dir = NULL, *pos[2] = { NULL, NULL };
	int force = 0, open_it = 0, np = 0;

	load();
	if (argc == 1) {
		if (isatty(STDIN_FILENO))
			return wizard();
		usage();
		return 2;
	}
	if (!strcmp(argv[1], "lista") || !strcmp(argv[1], "list")) {
		if (argc > 2 && !strcmp(argv[2], "--json"))
			list_json();
		else
			list();
		return 0;
	}
	if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help") || !strcmp(argv[1], "ayuda")) {
		usage();
		return 0;
	}
	if (!strcmp(argv[1], "exportar") || !strcmp(argv[1], "export"))
		return argc == 3 ? export_to(argv[2]) : (usage(), 2);
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--dir") && i + 1 < argc)
			dir = argv[++i];
		else if (!strcmp(argv[i], "--force"))
			force = 1;
		else if (!strcmp(argv[i], "--abrir") || !strcmp(argv[i], "--open"))
			open_it = 1;
		else if (np < 2)
			pos[np++] = argv[i];
		else {
			usage();
			return 2;
		}
	}
	if (np != 2) {
		usage();
		return 2;
	}
	{
		const struct tpl *t = find(pos[0]);
		char made[1100];
		int rc;

		if (!t) {
			fprintf(stderr, "rros-new: no hay ninguna plantilla '%s' (mira: rros-new lista)\n", pos[0]);
			return 2;
		}
		rc = create(t, pos[1], dir, force, made, sizeof made);
		if (rc)
			return rc;
		printf("Creado: %s\n", made);
		if (open_it)
			open_path(made);
		return 0;
	}
}
