/* rros-settings api: la parte de RROS Ajustes que lee y cambia el sistema. La usa la aplicación nativa
 * (rros-ajustes): «rros-settings api MÉTODO RUTA [CUERPO]» escribe la respuesta (JSON) en la salida estándar.
 * El cuerpo va como un formulario (clave=valor&...). Sin servidor ni navegador: nada escucha en la red. */
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "rros_profile.h"
#include "rros_userconf.h"
#include "settings.h"
#include "rros_schema.h"
#include "rros_security.h"
#include "theme.h"

#define MAX_BODY 8192

struct req {
	char method[8], path[256], query[256];
	char body[MAX_BODY + 1];
};

static int apply_on_set = 1;
static int last_code = 200;

/* ---- utilidades HTTP ---- */

static void respond(int fd, int code, const char *ctype, const char *body, size_t len, const char *extra)
{
	(void)ctype; (void)extra;
	last_code = code;
	while (len) {
		ssize_t w = write(fd, body, len);

		if (w <= 0)
			return;
		body += w;
		len -= (size_t)w;
	}
}

static void respond_json(int fd, int code, const char *json)
{
	respond(fd, code, "application/json; charset=utf-8", json, strlen(json), NULL);
}

static void respond_ok(int fd, int ok, const char *err)
{
	struct sbuf b = { 0 };

	sb_put(&b, ok ? "{\"ok\":true}" : "{\"ok\":false,\"error\":");
	if (!ok) {
		sb_json(&b, err);
		sb_put(&b, "}");
	}
	respond_json(fd, ok ? 200 : 400, b.p ? b.p : "{}");
	sb_free(&b);
}

static int hexv(int c)
{
	return isdigit(c) ? c - '0' : (tolower(c) - 'a' + 10);
}

/* Valor de `key` en un cuerpo application/x-www-form-urlencoded. */
static int form_get(const char *body, const char *key, char *out, size_t n)
{
	size_t kl = strlen(key);
	const char *p = body;

	while (*p) {
		const char *amp = strchr(p, '&');
		size_t seg = amp ? (size_t)(amp - p) : strlen(p);

		if (seg > kl && !strncmp(p, key, kl) && p[kl] == '=') {
			const char *v = p + kl + 1, *end = p + seg;
			size_t o = 0;

			while (v < end && o + 1 < n) {
				if (*v == '%' && v + 2 < end && isxdigit((unsigned char)v[1]) &&
				    isxdigit((unsigned char)v[2])) {
					out[o++] = (char)(hexv(v[1]) * 16 + hexv(v[2]));
					v += 3;
				} else if (*v == '+') {
					out[o++] = ' ';
					v++;
				} else {
					out[o++] = *v++;
				}
			}
			out[o] = '\0';
			return 0;
		}
		p += seg;
		if (amp)
			p++;
	}
	out[0] = '\0';
	return -1;
}

/* ---- rutas ---- */

static void do_state(int fd)
{
	struct model m;
	struct sbuf b = { 0 };

	if (model_load(&m) != 0) {
		respond_json(fd, 400, "{\"ok\":false,\"error\":\"no se pudo cargar el perfil\"}");
		model_free(&m);
		return;
	}
	if (model_state_json(&m, &b) == 0)
		respond_json(fd, 200, b.p);
	else
		respond_json(fd, 400, "{\"ok\":false,\"error\":\"sin memoria\"}");
	sb_free(&b);
	model_free(&m);
}

/* en segundo plano: la ventana no espera (generar un fondo nuevo tarda medio segundo) */
static void apply_desktop(void)
{
	char *args[] = { "rros-theme", "apply", "--quiet", NULL };

	spawn_detached("rros-theme", args);
}

static int needs_apply(const char *key)
{
	static const char *cats[] = { "ui.", "panel.", "term.", "input.", "keybind.", "rule.", "autostart.", "access." };

	for (size_t i = 0; i < sizeof cats / sizeof *cats; i++)
		if (!strncmp(key, cats[i], strlen(cats[i])))
			return 1;
	return 0;
}

static void do_export(int fd)
{
	char user[1024], *buf = NULL;
	FILE *f;
	long sz;

	if (rros_user_conf(user, sizeof user) || !(f = fopen(user, "r"))) {
		respond(fd, 200, "text/plain; charset=utf-8", "", 0,
			"Content-Disposition: attachment; filename=\"rros.toml\"\r\n");
		return;
	}
	fseek(f, 0, SEEK_END);
	sz = ftell(f);
	rewind(f);
	buf = malloc((size_t)sz + 1);
	if (buf) {
		sz = (long)fread(buf, 1, (size_t)sz, f);
		respond(fd, 200, "text/plain; charset=utf-8", buf, (size_t)sz,
			"Content-Disposition: attachment; filename=\"rros.toml\"\r\n");
		free(buf);
	}
	fclose(f);
}

static void do_apps(int fd)
{
	struct sbuf b = { 0 };
	char *args[] = { "rros-apps", "list", "--json", NULL };

	if (run_capture("rros-apps", args, &b) == 0 && b.p)
		respond_json(fd, 200, b.p);
	else
		respond_json(fd, 400, "{\"ok\":false,\"error\":\"no se pudo leer el catálogo de aplicaciones\"}");
	sb_free(&b);
}

/* Lo que necesita ser root se ejecuta con pkexec (Plasma pide la contraseña en su ventana): `pkexec sh -c SCRIPT
 * rros ARG...`. Los argumentos van separados, nunca dentro del script. Espera a que termine. */
static int admin_run(const char *script, const char *a1, const char *a2, char *err, size_t en)
{
	char *argv[8];
	int n = 0, rc;
	struct sbuf out = { 0 };

	argv[n++] = "pkexec";
	argv[n++] = "sh";
	argv[n++] = "-c";
	argv[n++] = (char *)script;
	argv[n++] = "rros";
	if (a1)
		argv[n++] = (char *)a1;
	if (a2)
		argv[n++] = (char *)a2;
	argv[n] = NULL;
	rc = run_capture_all("pkexec", argv, &out);
	if (rc == 126 || rc == 127)
		snprintf(err, en, "no se ha dado permiso (hace falta la contraseña de un administrador)");
	else if (rc != 0)
		snprintf(err, en, "%.250s", out.p && *out.p ? out.p : "falló la orden");
	sb_free(&out);
	return rc == 0 ? 0 : -1;
}

/* Ajustes del sistema (los aplica rros-apply como root): se guardan en /etc/rros/rros.toml */
static int sys_set(const char *key, const char *val, int unset, char *err, size_t en)
{
	if (!rros_key_system_scope(key)) {
		snprintf(err, en, "'%.120s' no es un ajuste del sistema", key);
		return -1;
	}
	if (unset) {
		if (!rros_schema_find(key)) {
			snprintf(err, en, "clave '%.120s' desconocida", key);
			return -1;
		}
		return admin_run("rros-config unset --system \"$1\" && rros-apply --services", key, NULL, err, en);
	}
	if (rros_user_check(key, val, err, en))
		return -1;
	return admin_run("rros-config set --system \"$1\" \"$2\" && rros-apply --services", key, val, err, en);
}

static void respond_capture(int fd, const char *tool, char *const argv[], const char *what)
{
	struct sbuf b = { 0 };
	char msg[200];

	if (run_capture(tool, argv, &b) == 0 && b.p) {
		respond_json(fd, 200, b.p);
	} else {
		snprintf(msg, sizeof msg, "{\"ok\":false,\"error\":\"no se pudo leer %s\"}", what);
		respond_json(fd, 400, msg);
	}
	sb_free(&b);
}

/* ejecuta una herramienta y responde {ok, output} con su salida (y sus errores) */
static void respond_run(int fd, const char *tool, char *const argv[])
{
	struct sbuf b = { 0 }, o = { 0 };
	int rc = run_capture_all(tool, argv, &b);

	sb_printf(&o, "{\"ok\":%s,\"output\":", rc == 0 ? "true" : "false");
	sb_json(&o, b.p ? b.p : "");
	sb_put(&o, "}");
	respond_json(fd, 200, o.p ? o.p : "{\"ok\":false}");
	sb_free(&b);
	sb_free(&o);
}

/* miniatura de un estilo de fondo con los colores del tema actual (PNG) */
static void do_wallpaper_preview(int fd, const char *query)
{
	static const char *const styles[] = { "aurora", "ondas", "degradado", "circulos" };
	char style[16] = "", path[600], *body = NULL;
	const char *dir = getenv("XDG_RUNTIME_DIR");
	const char *st = NULL;
	FILE *f;
	long len;

	form_get(query, "style", style, sizeof style);
	for (size_t i = 0; i < sizeof styles / sizeof *styles; i++)
		if (!strcmp(style, styles[i]))
			st = styles[i];
	if (!st) {
		respond_json(fd, 404, "{\"ok\":false,\"error\":\"estilo desconocido\"}");
		return;
	}
	snprintf(path, sizeof path, "%s/rros-fondo-%s.png", dir && *dir ? dir : "/tmp", st);
	{
		char *args[] = { "rros-theme", "wallpaper", "--style", (char *)st, "--size", "384x216", "--out", path, NULL };

		if (run_tool("rros-theme", args, 1) != 0) {
			respond_json(fd, 500, "{\"ok\":false,\"error\":\"no se pudo generar el fondo\"}");
			return;
		}
	}
	{
		struct sbuf o = { 0 };

		sb_put(&o, "{\"ok\":true,\"path\":");
		sb_json(&o, path);
		sb_put(&o, "}");
		respond_json(fd, 200, o.p ? o.p : "{}");
		sb_free(&o);
	}
	(void)f; (void)len; (void)body;
}

static int dev_id_valid(const char *s)
{
	size_t n = strlen(s);

	if (n == 0 || n > 40)
		return 0;
	for (; *s; s++)
		if (!((*s >= 'a' && *s <= 'z') || (*s >= '0' && *s <= '9') || *s == '-' || *s == '_' || *s == ':' || *s == '.'))
			return 0;
	return 1;
}

/* nombre o carpeta escritos por el usuario: sin caracteres de control ni guion inicial (no son opciones) */
static int text_arg_ok(const char *s, size_t max)
{
	if (!*s || *s == '-' || strlen(s) > max)
		return 0;
	for (; *s; s++)
		if ((unsigned char)*s < 0x20)
			return 0;
	return 1;
}

static int handle(int fd, struct req *rp)
{
	struct req r = *rp;
	char a[400], b[400], c[400], err[300];

	if (!strcmp(r.method, "GET") && !strcmp(r.path, "/api/state")) {
		do_state(fd);
	} else if (!strcmp(r.method, "GET") && !strcmp(r.path, "/api/export")) {
		do_export(fd);
	} else if (!strcmp(r.method, "GET") && !strcmp(r.path, "/api/apps")) {
		do_apps(fd);
	} else if (!strcmp(r.method, "GET") && !strcmp(r.path, "/api/security")) {
		char *args[] = { "rros-security", "auditoria", "--json", NULL };

		respond_capture(fd, "rros-security", args, "el estado de seguridad");
	} else if (!strcmp(r.method, "GET") && !strcmp(r.path, "/api/devices")) {
		char *args[] = { "rros-devices", "lista", "--json", NULL };

		respond_capture(fd, "rros-devices", args, "los dispositivos");
	} else if (!strcmp(r.method, "GET") && !strcmp(r.path, "/api/install_disks")) {
		char *args[] = { "rros-installer", "disks", "--json", NULL };

		respond_capture(fd, "rros-installer", args, "los discos");
	} else if (!strcmp(r.method, "GET") && !strcmp(r.path, "/api/install_status")) {
		struct sbuf st = { 0 };

		install_status_json(&st);
		respond_json(fd, 200, st.p ? st.p : "{}");
		sb_free(&st);
	} else if (!strcmp(r.method, "GET") && !strcmp(r.path, "/api/wallpaper_preview")) {
		do_wallpaper_preview(fd, r.query);
	} else if (!strcmp(r.method, "GET") && !strcmp(r.path, "/api/templates")) {
		char *args[] = { "rros-new", "lista", "--json", NULL };

		respond_capture(fd, "rros-new", args, "las plantillas");
	} else if (strcmp(r.method, "POST") != 0) {
		respond_json(fd, 404, "{\"ok\":false,\"error\":\"ruta desconocida\"}");
	} else if (!strcmp(r.path, "/api/apply")) {
		apply_desktop();
		respond_ok(fd, 1, "");
	} else if (!strcmp(r.path, "/api/set") || !strcmp(r.path, "/api/unset")) {
		int unset = !strcmp(r.path, "/api/unset");

		form_get(r.body, "key", a, sizeof a);
		form_get(r.body, "value", b, sizeof b);
		if (unset ? rros_user_unset(a, 0, err, sizeof err) : rros_user_set(a, b, 0, err, sizeof err)) {
			respond_ok(fd, 0, err);
		} else {
			if (apply_on_set && needs_apply(a))
				apply_desktop();
			respond_ok(fd, 1, "");
		}
	} else if (!strcmp(r.path, "/api/sys_set") || !strcmp(r.path, "/api/sys_unset")) {
		form_get(r.body, "key", a, sizeof a);
		form_get(r.body, "value", b, sizeof b);
		respond_ok(fd, sys_set(a, b, !strcmp(r.path, "/api/sys_unset"), err, sizeof err) == 0, err);
	} else if (!strcmp(r.path, "/api/security_level")) {
		form_get(r.body, "level", a, sizeof a);
		if (!rros_seclevel_find(a))
			respond_ok(fd, 0, "nivel desconocido");
		else
			respond_ok(fd, admin_run("rros-security nivel \"$1\"", a, NULL, err, sizeof err) == 0, err);
	} else if (!strcmp(r.path, "/api/security_port")) {
		form_get(r.body, "action", a, sizeof a);
		form_get(r.body, "port", b, sizeof b);
		if (strcmp(a, "abrir") && strcmp(a, "cerrar"))
			respond_ok(fd, 0, "acción desconocida");
		else if (!*b || strchr(b, ',') || rros_fw_ports_check(b, err, sizeof err))
			respond_ok(fd, 0, *err ? err : "indica un puerto, p. ej. 8080/tcp");
		else
			respond_ok(fd, admin_run("rros-security \"$1\" \"$2\"", a, b, err, sizeof err) == 0, err);
	} else if (!strcmp(r.path, "/api/device_action")) {
		form_get(r.body, "action", a, sizeof a);
		form_get(r.body, "id", b, sizeof b);
		if (!dev_id_valid(b)) {
			respond_ok(fd, 0, "dispositivo no válido");
		} else if (!strcmp(a, "mount") || !strcmp(a, "unmount") || !strcmp(a, "eject")) {
			char *args[] = { "rros-devices", !strcmp(a, "mount") ? "montar" : !strcmp(a, "unmount") ? "desmontar" : "expulsar", b, NULL };

			respond_run(fd, "rros-devices", args);
		} else if (!strcmp(a, "block") || !strcmp(a, "unblock")) {
			char *args[] = { "rros-devices", "radio", b, !strcmp(a, "block") ? "off" : "on", NULL };

			respond_run(fd, "rros-devices", args);
		} else {
			respond_ok(fd, 0, "acción desconocida");
		}
	} else if (!strcmp(r.path, "/api/desktop")) {
		form_get(r.body, "name", a, sizeof a);
		if (!rros_desktop_find(a)) {
			respond_ok(fd, 0, "estilo de escritorio desconocido");
		} else {
			char *args[] = { "rros-theme", "escritorio", a, NULL };

			respond_run(fd, "rros-theme", args);
		}
	} else if (!strcmp(r.path, "/api/a11y")) {
		form_get(r.body, "what", a, sizeof a);
		if (strcmp(a, "lector") && strcmp(a, "teclado")) {
			respond_ok(fd, 0, "ayuda desconocida");
		} else {
			char *args[] = { "rros-a11y", a, NULL };

			respond_ok(fd, spawn_detached("rros-a11y", args) == 0, "no se pudo iniciar");
		}
	} else if (!strcmp(r.path, "/api/install")) {
		struct install_req ir;

		memset(&ir, 0, sizeof ir);
		form_get(r.body, "disk", ir.disk, sizeof ir.disk);
		form_get(r.body, "mode", ir.mode, sizeof ir.mode);
		form_get(r.body, "user", ir.user, sizeof ir.user);
		form_get(r.body, "fullname", ir.fullname, sizeof ir.fullname);
		form_get(r.body, "hostname", ir.host, sizeof ir.host);
		form_get(r.body, "keymap", ir.keymap, sizeof ir.keymap);
		form_get(r.body, "locale", ir.locale, sizeof ir.locale);
		form_get(r.body, "timezone", ir.tz, sizeof ir.tz);
		form_get(r.body, "password", ir.password, sizeof ir.password);
		form_get(r.body, "passphrase", ir.passphrase, sizeof ir.passphrase);
		form_get(r.body, "cmdline", ir.cmdline, sizeof ir.cmdline);
		form_get(r.body, "encrypt", a, sizeof a);
		ir.encrypt = !strcmp(a, "1");
		if (!model_is_live())
			respond_ok(fd, 0, "solo se instala desde el sistema live (la ISO o la memoria USB)");
		else
			respond_ok(fd, install_start(&ir, err, sizeof err) == 0, err);
		/* que las contraseñas no queden en memoria (el cuerpo de la petición tampoco) */
		memset(&ir, 0, sizeof ir);
		memset(r.body, 0, sizeof r.body);
	} else if (!strcmp(r.path, "/api/reboot")) {
		char *args[] = { "systemctl", "reboot", NULL };

		if (install_running())
			respond_ok(fd, 0, "espera a que termine la instalación");
		else
			respond_ok(fd, spawn_detached("systemctl", args) == 0, "no se pudo reiniciar");
	} else if (!strcmp(r.path, "/api/new_file")) {
		char dir[400];

		form_get(r.body, "template", a, sizeof a);
		form_get(r.body, "name", b, sizeof b);
		form_get(r.body, "dir", dir, sizeof dir);
		if (!text_arg_ok(a, 60) || !text_arg_ok(b, 200) || (*dir && !text_arg_ok(dir, 390))) {
			respond_ok(fd, 0, "plantilla, nombre o carpeta no válidos");
		} else {
			char *args[] = { "rros-new", a, b, "--abrir", *dir ? "--dir" : NULL, dir, NULL };

			respond_run(fd, "rros-new", args);
		}
	} else if (!strcmp(r.path, "/api/theme_new")) {
		form_get(r.body, "name", a, sizeof a);
		form_get(r.body, "base", b, sizeof b);
		if (rros_theme_new(a, b[0] ? b : "aurora", err, sizeof err))
			respond_ok(fd, 0, err);
		else
			respond_ok(fd, 1, "");
	} else if (!strcmp(r.path, "/api/theme_color")) {
		form_get(r.body, "name", a, sizeof a);
		form_get(r.body, "variant", b, sizeof b);
		form_get(r.body, "key", c, sizeof c);
		{
			char v[40];

			form_get(r.body, "value", v, sizeof v);
			if (rros_theme_set_color(a, b, c, v, err, sizeof err)) {
				respond_ok(fd, 0, err);
			} else {
				if (apply_on_set)
					apply_desktop();
				respond_ok(fd, 1, "");
			}
		}
	} else if (!strcmp(r.path, "/api/snapshot")) {
		int rc;

		form_get(r.body, "action", a, sizeof a);
		form_get(r.body, "name", b, sizeof b);
		rc = !strcmp(a, "save") ? rros_snapshot_save(b, err, sizeof err) :
		     !strcmp(a, "restore") ? rros_snapshot_restore(b, err, sizeof err) :
		     !strcmp(a, "delete") ? rros_snapshot_delete(b, err, sizeof err) :
		     (snprintf(err, sizeof err, "acción desconocida"), -1);
		if (rc == 0 && !strcmp(a, "restore") && apply_on_set)
			apply_desktop();
		respond_ok(fd, rc == 0, err);
	} else {
		respond_json(fd, 404, "{\"ok\":false,\"error\":\"ruta desconocida\"}");
	}
	return 0;
}

/* rros-settings api MÉTODO RUTA [CUERPO]: 0 si la respuesta es correcta */
int run_api(const char *method, const char *target, const char *body)
{
	struct req r;
	const char *q = strchr(target, '?');
	size_t pl = q ? (size_t)(q - target) : strlen(target);

	memset(&r, 0, sizeof r);
	signal(SIGPIPE, SIG_IGN);
	if (strlen(method) >= sizeof r.method || pl >= sizeof r.path || (q && strlen(q + 1) >= sizeof r.query) ||
	    (body && strlen(body) > MAX_BODY)) {
		fputs("{\"ok\":false,\"error\":\"petición demasiado larga\"}\n", stdout);
		return 2;
	}
	snprintf(r.method, sizeof r.method, "%s", method);
	memcpy(r.path, target, pl);
	if (q)
		snprintf(r.query, sizeof r.query, "%s", q + 1);
	if (body)
		snprintf(r.body, sizeof r.body, "%s", body);
	if (getenv("RROS_NO_APPLY"))
		apply_on_set = 0;
	fflush(stdout);
	handle(STDOUT_FILENO, &r);
	respond(STDOUT_FILENO, last_code, "", "\n", 1, NULL);
	memset(&r, 0, sizeof r);   /* contraseñas del instalador */
	return last_code < 400 ? 0 : 1;
}
