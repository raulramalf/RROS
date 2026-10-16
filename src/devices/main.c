/* rros-devices: administración de dispositivos.
 *   rros-devices [lista] [CATEGORÍA] [--json]   inventario (discos, USB, red, radios, sonido...)
 *   rros-devices montar|desmontar PARTICIÓN     p. ej. sdb1 (con udisks: sin sudo)
 *   rros-devices expulsar DISCO                 desmonta y apaga una memoria USB o abre el lector
 *   rros-devices radio wifi|bluetooth|todo on|off
 * RROS_ROOT antepone un directorio a /sys, /proc y /run (pruebas). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "rros_devices.h"

static int run(const char *const argv[])
{
	pid_t pid;
	int st;

	fflush(stdout);
	pid = fork();
	if (pid < 0)
		return -1;
	if (pid == 0) {
		execvp(argv[0], (char *const *)argv);
		fprintf(stderr, "rros-devices: no se pudo ejecutar %s\n", argv[0]);
		_exit(127);
	}
	if (waitpid(pid, &st, 0) < 0)
		return -1;
	return WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}

static int has_action(const struct rros_dev *d, const char *act)
{
	char copy[48], *save = NULL;

	snprintf(copy, sizeof copy, "%s", d->actions);
	for (char *t = strtok_r(copy, ",", &save); t; t = strtok_r(NULL, ",", &save))
		if (!strcmp(t, act))
			return 1;
	return 0;
}

static const char *cat_label(const char *cat)
{
	for (size_t i = 0; i < rros_devcats_len; i++)
		if (!strcmp(rros_devcats[i][0], cat))
			return rros_devcats[i][1];
	return cat;
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

static void list_json(const struct rros_devlist *l, const char *only)
{
	int first = 1;

	fputs("{\"categories\":[", stdout);
	for (size_t i = 0; i < rros_devcats_len; i++) {
		printf("%s{\"id\":", i ? "," : "");
		json_str(rros_devcats[i][0]);
		fputs(",\"label\":", stdout);
		json_str(rros_devcats[i][1]);
		putchar('}');
	}
	fputs("],\"devices\":[", stdout);
	for (size_t i = 0; i < l->n; i++) {
		const struct rros_dev *d = &l->v[i];
		const char *f[][2] = { { "cat", d->cat }, { "id", d->id }, { "parent", d->parent }, { "name", d->name },
				       { "detail", d->detail }, { "state", d->state }, { "actions", d->actions } };

		if (only && strcmp(d->cat, only))
			continue;
		fputs(first ? "{" : ",{", stdout);
		first = 0;
		for (size_t k = 0; k < sizeof f / sizeof *f; k++) {
			printf("%s\"%s\":", k ? "," : "", f[k][0]);
			json_str(f[k][1]);
		}
		putchar('}');
	}
	puts("]}");
}

static void list_text(const struct rros_devlist *l, const char *only)
{
	int color = isatty(STDOUT_FILENO) && !getenv("NO_COLOR");
	const char *b = color ? "\033[1m" : "", *dim = color ? "\033[2m" : "", *r = color ? "\033[0m" : "";

	for (size_t c = 0; c < rros_devcats_len; c++) {
		int any = 0;

		if (only && strcmp(rros_devcats[c][0], only))
			continue;
		for (size_t i = 0; i < l->n; i++) {
			const struct rros_dev *d = &l->v[i];

			if (strcmp(d->cat, rros_devcats[c][0]))
				continue;
			if (!any)
				printf("%s%s%s\n", b, rros_devcats[c][1], r);
			any = 1;
			printf("  %s%-10s%s %s%s%s%s%s%s", d->parent[0] ? "  " : "", d->id, "", d->name,
			       *d->detail ? dim : "", *d->detail ? " · " : "", d->detail, *d->detail ? r : "", "");
			if (*d->state)
				printf("  [%s]", d->state);
			if (*d->actions)
				printf("  %s→ %s%s", dim, d->actions, r);
			putchar('\n');
		}
	}
}

static int valid_id(const char *s)
{
	if (!*s || strlen(s) > 40)
		return 0;
	for (; *s; s++)
		if (!((*s >= 'a' && *s <= 'z') || (*s >= '0' && *s <= '9') || *s == '-' || *s == '_' || *s == ':' || *s == '.'))
			return 0;
	return 1;
}

static int act_volume(const struct rros_devlist *l, const char *id, int mount)
{
	const struct rros_dev *d = valid_id(id) ? rros_devs_find(l, id) : NULL;
	char dev[64];

	if (!d || strcmp(d->cat, "disco")) {
		fprintf(stderr, "rros-devices: no hay ningún disco o partición '%s' (mira: rros-devices lista disco)\n", id);
		return 2;
	}
	if (!has_action(d, mount ? "mount" : "unmount")) {
		fprintf(stderr, "rros-devices: no se puede %s %s (%s)\n", mount ? "montar" : "desmontar", id,
			*d->state ? d->state : "sin sistema de archivos");
		return 1;
	}
	snprintf(dev, sizeof dev, "/dev/%s", id);
	{
		const char *argv[] = { "udisksctl", mount ? "mount" : "unmount", "-b", dev, NULL };

		return run(argv) == 0 ? 0 : 1;
	}
}

static int act_eject(const struct rros_devlist *l, const char *id)
{
	const struct rros_dev *d = valid_id(id) ? rros_devs_find(l, id) : NULL;
	char dev[64];

	if (!d || strcmp(d->cat, "disco") || d->parent[0]) {
		fprintf(stderr, "rros-devices: '%s' no es un disco (usa el disco, p. ej. sdb, no la partición)\n", id);
		return 2;
	}
	if (!has_action(d, "eject")) {
		fprintf(stderr, "rros-devices: %s no es extraíble\n", id);
		return 1;
	}
	/* primero se desmonta todo lo suyo; si algo es del sistema, no se toca */
	for (size_t i = 0; i < l->n; i++) {
		const struct rros_dev *p = &l->v[i];

		if (strcmp(p->parent, id) && strcmp(p->id, id))
			continue;
		if (!strncmp(p->state, "sistema:", 8)) {
			fprintf(stderr, "rros-devices: %s está en uso por el sistema (%s)\n", p->id, p->state);
			return 1;
		}
		if (has_action(p, "unmount")) {
			char pdev[64];
			const char *argv[] = { "udisksctl", "unmount", "-b", pdev, NULL };

			snprintf(pdev, sizeof pdev, "/dev/%s", p->id);
			if (run(argv) != 0)
				return 1;
		}
	}
	snprintf(dev, sizeof dev, "/dev/%s", id);
	if (!strncmp(id, "sr", 2)) {
		const char *argv[] = { "eject", dev, NULL };

		return run(argv) == 0 ? 0 : 1;
	} else {
		const char *argv[] = { "udisksctl", "power-off", "-b", dev, NULL };
		int rc = run(argv);

		if (rc == 0)
			printf("Ya puedes desconectar %s.\n", d->name);
		return rc == 0 ? 0 : 1;
	}
}

static int act_radio(const struct rros_devlist *l, const char *what, const char *onoff)
{
	const char *type = NULL;
	char idx[16] = "";
	int on;

	if (!strcmp(onoff, "on") || !strcmp(onoff, "activar") || !strcmp(onoff, "si"))
		on = 1;
	else if (!strcmp(onoff, "off") || !strcmp(onoff, "desactivar") || !strcmp(onoff, "no"))
		on = 0;
	else {
		fprintf(stderr, "rros-devices: usa on u off\n");
		return 2;
	}
	if (!strcmp(what, "wifi"))
		type = "wifi";
	else if (!strcmp(what, "bluetooth"))
		type = "bluetooth";
	else if (!strcmp(what, "todo") || !strcmp(what, "all"))
		type = "all";
	else if (!strncmp(what, "rfkill", 6) && valid_id(what) && rros_devs_find(l, what))
		snprintf(idx, sizeof idx, "%s", what + 6);
	else {
		fprintf(stderr, "rros-devices: radio desconocida '%s' (wifi, bluetooth, todo o rfkillN)\n", what);
		return 2;
	}
	{
		const char *argv[] = { "rfkill", on ? "unblock" : "block", *idx ? idx : type, NULL };

		if (run(argv) != 0)
			return 1;
	}
	printf("%s %s\n", *idx ? what : what, on ? "activado" : "desactivado");
	return 0;
}

static void usage(void)
{
	fputs("uso: rros-devices [COMANDO]\n"
	      "  lista [CATEGORÍA] [--json]   dispositivos; categorías: disco usb red radio sonido pantalla\n"
	      "                               energia entrada camara tarjeta (por defecto: todas)\n"
	      "  montar PARTICIÓN             monta una memoria o partición (p. ej. sdb1)\n"
	      "  desmontar PARTICIÓN          la desmonta\n"
	      "  expulsar DISCO               desmonta todo y apaga la memoria USB (p. ej. sdb)\n"
	      "  radio wifi|bluetooth|todo on|off\n", stderr);
}

int main(int argc, char **argv)
{
	struct rros_devlist l;
	const char *root = getenv("RROS_ROOT") ? getenv("RROS_ROOT") : "";
	const char *cmd = argc > 1 ? argv[1] : "lista";
	int rc = 0;

	if (!strcmp(cmd, "-h") || !strcmp(cmd, "--help") || !strcmp(cmd, "ayuda")) {
		usage();
		return 0;
	}
	rros_devs_scan(&l, root);
	if (!strcmp(cmd, "lista") || !strcmp(cmd, "list") || !strcmp(cmd, "--json")) {
		const char *only = NULL;
		int json = !strcmp(cmd, "--json");

		for (int i = 2; i < argc; i++) {
			if (!strcmp(argv[i], "--json"))
				json = 1;
			else
				only = argv[i];
		}
		if (only && strcmp(cat_label(only), only) == 0) {
			fprintf(stderr, "rros-devices: categoría desconocida '%s'\n", only);
			rc = 2;
		} else if (json) {
			list_json(&l, only);
		} else {
			list_text(&l, only);
		}
	} else if ((!strcmp(cmd, "montar") || !strcmp(cmd, "mount") || !strcmp(cmd, "desmontar") ||
		    !strcmp(cmd, "unmount")) && argc == 3) {
		rc = act_volume(&l, argv[2], !strcmp(cmd, "montar") || !strcmp(cmd, "mount"));
	} else if ((!strcmp(cmd, "expulsar") || !strcmp(cmd, "eject")) && argc == 3) {
		rc = act_eject(&l, argv[2]);
	} else if (!strcmp(cmd, "radio") && argc == 4) {
		rc = act_radio(&l, argv[2], argv[3]);
	} else {
		usage();
		rc = 2;
	}
	rros_devs_free(&l);
	return rc;
}
