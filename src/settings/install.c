/* Instalación desde la GUI (sistema live): lanza `sudo -n rros-install` en segundo plano y lee su progreso.
 * Las contraseñas van en archivos 0600 de $XDG_RUNTIME_DIR (nunca en la línea de órdenes) y se borran al
 * terminar. El proceso sigue aunque se cierre la ventana; al volver a abrirla se ve cómo va. */
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "settings.h"

static void path_of(char *out, size_t n, const char *what)
{
	const char *d = getenv("XDG_RUNTIME_DIR");

	snprintf(out, n, "%s/rros-install.%s", d && *d ? d : "/tmp", what);
}

static int charset_ok(const char *s, const char *extra, size_t min, size_t max)
{
	size_t n = strlen(s);

	if (n < min || n > max)
		return 0;
	for (; *s; s++)
		if (!isalnum((unsigned char)*s) && !strchr(extra, *s))
			return 0;
	return 1;
}

static int no_ctrl(const char *s, const char *bad, size_t max)
{
	if (strlen(s) > max)
		return 0;
	for (; *s; s++)
		if ((unsigned char)*s < 0x20 || (unsigned char)*s == 0x7f || strchr(bad, *s))
			return 0;
	return 1;
}

static int write_secret(const char *path, const char *val)
{
	int fd;
	size_t n = strlen(val);

	unlink(path);
	fd = open(path, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
	if (fd < 0)
		return -1;
	if (write(fd, val, n) != (ssize_t)n || write(fd, "\n", 1) != 1) {
		close(fd);
		unlink(path);
		return -1;
	}
	return close(fd);
}

static int exists(const char *p)
{
	struct stat st;

	return stat(p, &st) == 0;
}

int install_running(void)
{
	char log[512], rc[512];

	path_of(log, sizeof log, "log");
	path_of(rc, sizeof rc, "rc");
	return exists(log) && !exists(rc);
}

int install_start(const struct install_req *r, char *err, size_t en)
{
	char log[512], rc[512], pw[512], pp[512], prog[512];
	char *argv[48];
	int n = 0, fd;

	if (!charset_ok(r->disk + (strncmp(r->disk, "/dev/", 5) ? 0 : 5), "", 2, 30) || strncmp(r->disk, "/dev/", 5)) {
		snprintf(err, en, "elige un disco");
		return -1;
	}
	if (strcmp(r->mode, "wipe") && strcmp(r->mode, "alongside")) {
		snprintf(err, en, "modo de instalación desconocido");
		return -1;
	}
	if (!charset_ok(r->user, "_-", 1, 31) || !islower((unsigned char)r->user[0])) {
		snprintf(err, en, "nombre de usuario no válido: minúsculas, cifras, - y _, empezando por una letra");
		return -1;
	}
	if (!no_ctrl(r->fullname, ":,=", 60)) {
		snprintf(err, en, "el nombre completo no puede llevar : , ni =");
		return -1;
	}
	if (!charset_ok(r->host, "-", 1, 63) || r->host[0] == '-') {
		snprintf(err, en, "nombre del equipo no válido: letras, cifras y -");
		return -1;
	}
	if (!charset_ok(r->keymap, "_-", 2, 20) || !charset_ok(r->locale, "_.-", 5, 20) ||
	    !charset_ok(r->tz, "_+/-", 1, 40) || strstr(r->tz, "..")) {
		snprintf(err, en, "idioma, teclado o zona horaria no válidos");
		return -1;
	}
	if (*r->cmdline && !charset_ok(r->cmdline, "_=,.: /-", 1, 120)) {
		snprintf(err, en, "parámetros del kernel no válidos");
		return -1;
	}
	if (!*r->password || !no_ctrl(r->password, "", 200)) {
		snprintf(err, en, "escribe una contraseña para tu usuario");
		return -1;
	}
	if (r->encrypt && (strlen(r->passphrase) < 8 || !no_ctrl(r->passphrase, "", 200))) {
		snprintf(err, en, "la contraseña del cifrado debe tener al menos 8 caracteres");
		return -1;
	}
	if (install_running()) {
		snprintf(err, en, "ya hay una instalación en marcha");
		return -1;
	}
	path_of(log, sizeof log, "log");
	path_of(rc, sizeof rc, "rc");
	path_of(pw, sizeof pw, "pw");
	path_of(pp, sizeof pp, "pp");
	path_of(prog, sizeof prog, "progress");
	unlink(rc);
	unlink(pp);
	if (write_secret(pw, r->password) || (r->encrypt && write_secret(pp, r->passphrase))) {
		snprintf(err, en, "no se pudo preparar la instalación (%s)", strerror(errno));
		return -1;
	}
	unlink(prog);
	fd = open(prog, O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW | O_CLOEXEC, 0644);
	if (fd >= 0)
		close(fd);
	unlink(log);
	fd = open(log, O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW | O_CLOEXEC, 0644);
	if (fd < 0) {
		snprintf(err, en, "no se pudo crear %s", log);
		unlink(pw);
		unlink(pp);
		return -1;
	}
	close(fd);

	argv[n++] = "sh";
	argv[n++] = "-c";
	argv[n++] = "log=$1 rc=$2 pw=$3 pp=$4; shift 4; sudo -n rros-install \"$@\" >>\"$log\" 2>&1; "
		    "s=$?; rm -f \"$pw\" \"$pp\"; echo $s >\"$rc\"";
	argv[n++] = "rros-install";
	argv[n++] = log;
	argv[n++] = rc;
	argv[n++] = pw;
	argv[n++] = pp;
	argv[n++] = "--yes";
	argv[n++] = "--disk";
	argv[n++] = (char *)r->disk;
	if (!strcmp(r->mode, "alongside"))
		argv[n++] = "--alongside";
	if (r->encrypt) {
		argv[n++] = "--encrypt";
		argv[n++] = "--passphrase-file";
		argv[n++] = pp;
	}
	argv[n++] = "--user";
	argv[n++] = (char *)r->user;
	argv[n++] = "--fullname";
	argv[n++] = (char *)r->fullname;
	argv[n++] = "--hostname";
	argv[n++] = (char *)r->host;
	argv[n++] = "--password-file";
	argv[n++] = pw;
	argv[n++] = "--keymap";
	argv[n++] = (char *)r->keymap;
	argv[n++] = "--locale";
	argv[n++] = (char *)r->locale;
	argv[n++] = "--timezone";
	argv[n++] = (char *)r->tz;
	if (*r->cmdline) {
		argv[n++] = "--cmdline";
		argv[n++] = (char *)r->cmdline;
	}
	argv[n++] = "--progress-file";
	argv[n++] = prog;
	argv[n] = NULL;
	if (spawn_detached("sh", argv) != 0) {
		snprintf(err, en, "no se pudo iniciar el instalador");
		unlink(pw);
		unlink(pp);
		unlink(log);
		return -1;
	}
	return 0;
}

/* lee como mucho n-1 bytes del final del archivo */
static void read_tail(const char *path, char *out, size_t n)
{
	FILE *f = fopen(path, "r");
	long size;
	size_t got;
	char *nl;

	*out = '\0';
	if (!f)
		return;
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	if (size > (long)n - 1) {
		fseek(f, size - (long)(n - 1), SEEK_SET);
	} else {
		fseek(f, 0, SEEK_SET);
	}
	got = fread(out, 1, n - 1, f);
	out[got] = '\0';
	fclose(f);
	/* empieza en una línea entera */
	if (size > (long)n - 1 && (nl = strchr(out, '\n')))
		memmove(out, nl + 1, strlen(nl + 1) + 1);
}

void install_status_json(struct sbuf *b)
{
	char log[512], rc[512], prog[512], line[300] = "", tail[3000], code[16] = "";
	int started, done, step = 0, steps = 0;
	const char *text = "", *error = "";
	FILE *f;

	path_of(log, sizeof log, "log");
	path_of(rc, sizeof rc, "rc");
	path_of(prog, sizeof prog, "progress");
	started = exists(log);
	done = exists(rc);
	if ((f = fopen(prog, "r"))) {
		if (fgets(line, sizeof line, f))
			line[strcspn(line, "\n")] = '\0';
		fclose(f);
	}
	if (!strncmp(line, "error ", 6)) {
		error = line + 6;
	} else {
		char *sp = strchr(line, ' ');

		if (sp && sscanf(line, "%d/%d", &step, &steps) == 2)
			text = sp + 1;
	}
	if (done && (f = fopen(rc, "r"))) {
		if (!fgets(code, sizeof code, f))
			*code = '\0';
		fclose(f);
	}
	read_tail(log, tail, sizeof tail);
	sb_printf(b, "{\"started\":%s,\"running\":%s,\"done\":%s,\"ok\":%s,\"step\":%d,\"steps\":%d,\"text\":",
		  started ? "true" : "false", started && !done ? "true" : "false", done ? "true" : "false",
		  done && atoi(code) == 0 && *code == '0' ? "true" : "false", step, steps);
	sb_json(b, text);
	sb_put(b, ",\"error\":");
	sb_json(b, error);
	sb_put(b, ",\"log\":");
	sb_json(b, tail);
	sb_put(b, "}");
}
