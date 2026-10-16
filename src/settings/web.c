/* GUI web local de rros-settings: servidor HTTP mínimo en 127.0.0.1 con token. */
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "rros_profile.h"
#include "rros_userconf.h"
#include "settings.h"
#include "rros_schema.h"
#include "rros_security.h"
#include "theme.h"

#define MAX_HEADERS 16384
#define MAX_BODY 8192

struct req {
	char method[8], path[256], query[256], host[128], origin[160], token[80];
	long clen;
	char body[MAX_BODY + 1];
};

static int port_g;
static char token_g[80];
static int apply_on_set = 1;
static char start_page_g[32];

/* página con la que se abre la ventana (#seguridad, #dispositivos...): solo letras y ':' */
int web_set_start_page(const char *p)
{
	size_t n = strlen(p);

	if (n == 0 || n >= sizeof start_page_g)
		return -1;
	for (size_t i = 0; i < n; i++)
		if (!((p[i] >= 'a' && p[i] <= 'z') || p[i] == ':'))
			return -1;
	snprintf(start_page_g, sizeof start_page_g, "%s", p);
	return 0;
}

/* ---- utilidades HTTP ---- */

static const char *status_text(int code)
{
	switch (code) {
	case 200: return "OK";
	case 400: return "Bad Request";
	case 403: return "Forbidden";
	case 404: return "Not Found";
	default:  return "Error";
	}
}

static void send_all(int fd, const char *buf, size_t n)
{
	while (n) {
		ssize_t w = send(fd, buf, n, MSG_NOSIGNAL);

		if (w <= 0)
			return;
		buf += w;
		n -= (size_t)w;
	}
}

static void respond(int fd, int code, const char *ctype, const char *body, size_t len, const char *extra)
{
	char head[512];
	int hl = snprintf(head, sizeof head,
			  "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\nCache-Control: no-store\r\n"
			  "X-Content-Type-Options: nosniff\r\nReferrer-Policy: no-referrer\r\n"
			  "Content-Security-Policy: default-src 'self' 'unsafe-inline'; img-src 'self' data:\r\n"
			  "%sConnection: close\r\n\r\n",
			  code, status_text(code), ctype, len, extra ? extra : "");

	send_all(fd, head, (size_t)hl);
	send_all(fd, body, len);
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

static int ieq(const char *a, const char *b)
{
	for (; *a && *b; a++, b++)
		if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
			return 0;
	return *a == *b;
}

/* ---- lectura de la petición ---- */

static int read_request(int fd, struct req *r)
{
	static char buf[MAX_HEADERS + MAX_BODY + 1];
	size_t n = 0, hend = 0;
	char *p, *line, *save = NULL, *q;
	struct pollfd pf = { fd, POLLIN, 0 };

	memset(r, 0, sizeof *r);
	while (n < MAX_HEADERS) {
		ssize_t k;

		if (poll(&pf, 1, 5000) <= 0)
			return -1;
		k = recv(fd, buf + n, MAX_HEADERS - n, 0);
		if (k <= 0)
			return -1;
		n += (size_t)k;
		buf[n] = '\0';
		if ((p = strstr(buf, "\r\n\r\n"))) {
			hend = (size_t)(p - buf) + 4;
			break;
		}
	}
	if (!hend)
		return -1;
	*strstr(buf, "\r\n\r\n") = '\0';
	line = strtok_r(buf, "\r\n", &save);
	if (!line)
		return -1;
	{
		char target[256];

		if (sscanf(line, "%7s %255s", r->method, target) != 2)
			return -1;
		if ((q = strchr(target, '?'))) {
			*q++ = '\0';
			if (strlen(q) >= sizeof r->query)
				return -1;
			snprintf(r->query, sizeof r->query, "%s", q);
		}
		if (strlen(target) >= sizeof r->path)
			return -1;
		snprintf(r->path, sizeof r->path, "%s", target);
	}
	while ((line = strtok_r(NULL, "\r\n", &save))) {
		char *colon = strchr(line, ':'), *v;

		if (!colon)
			continue;
		*colon = '\0';
		v = colon + 1;
		while (*v == ' ')
			v++;
		if (ieq(line, "Host"))
			snprintf(r->host, sizeof r->host, "%s", v);
		else if (ieq(line, "Origin"))
			snprintf(r->origin, sizeof r->origin, "%s", v);
		else if (ieq(line, "X-RROS-Token"))
			snprintf(r->token, sizeof r->token, "%s", v);
		else if (ieq(line, "Content-Length"))
			r->clen = atol(v);
	}
	if (r->clen < 0 || r->clen > MAX_BODY)
		return -1;
	if (r->clen) {
		/* los bytes del cuerpo que llegaron junto a las cabeceras están tras hend en el buffer original */
		size_t have = n > hend ? n - hend : 0;

		if (have > (size_t)r->clen)
			have = (size_t)r->clen;
		memcpy(r->body, buf + hend, have);
		while (have < (size_t)r->clen) {
			ssize_t k;

			if (poll(&pf, 1, 5000) <= 0)
				return -1;
			k = recv(fd, r->body + have, (size_t)r->clen - have, 0);
			if (k <= 0)
				return -1;
			have += (size_t)k;
		}
		r->body[r->clen] = '\0';
	}
	return 0;
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

/* ---- seguridad ---- */

static int host_ok(const char *h)
{
	char a[64], b[64];

	snprintf(a, sizeof a, "127.0.0.1:%d", port_g);
	snprintf(b, sizeof b, "localhost:%d", port_g);
	return ieq(h, a) || ieq(h, b);
}

static int origin_ok(const char *o)
{
	char a[96], b[96];

	if (!*o)
		return 1;
	snprintf(a, sizeof a, "http://127.0.0.1:%d", port_g);
	snprintf(b, sizeof b, "http://localhost:%d", port_g);
	return ieq(o, a) || ieq(o, b);
}

static int token_eq(const char *a)
{
	size_t la = strlen(a), lb = strlen(token_g), i;
	unsigned char d = (unsigned char)(la != lb);

	for (i = 0; i < lb; i++)
		d |= (unsigned char)((i < la ? a[i] : 0) ^ token_g[i]);
	return d == 0;
}

static int query_token_ok(const char *query)
{
	char t[80];

	return form_get(query, "t", t, sizeof t) == 0 && token_eq(t);
}

/* ---- contenido estático ---- */

static char *read_gui(size_t *len)
{
	const char *env = getenv("RROS_GUI_DIR");
	const char *dirs[4];
	char path[700];
	size_t nd = 0;

	if (env && *env)
		dirs[nd++] = env;
	dirs[nd++] = "/usr/share/rros/settings";
	dirs[nd++] = "gui";
	dirs[nd++] = "../gui";
	for (size_t i = 0; i < nd; i++) {
		FILE *f;
		long sz;
		char *buf;

		snprintf(path, sizeof path, "%s/index.html", dirs[i]);
		f = fopen(path, "r");
		if (!f)
			continue;
		fseek(f, 0, SEEK_END);
		sz = ftell(f);
		rewind(f);
		if (sz <= 0 || sz > 4 * 1024 * 1024 || !(buf = malloc((size_t)sz + 1))) {
			fclose(f);
			continue;
		}
		*len = fread(buf, 1, (size_t)sz, f);
		buf[*len] = '\0';
		fclose(f);
		return buf;
	}
	return NULL;
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

static void apply_desktop(void)
{
	char *args[] = { "rros-theme", "apply", "--quiet", NULL };

	run_tool("rros-theme", args, 1);
}

static int needs_apply(const char *key)
{
	static const char *cats[] = { "ui.", "panel.", "term.", "input.", "keybind.", "rule.", "autostart." };

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

static int id_valid(const char *id)
{
	size_t n = strlen(id);

	if (n == 0 || n > 39)
		return 0;
	for (size_t i = 0; i < n; i++)
		if (!((id[i] >= 'a' && id[i] <= 'z') || (id[i] >= '0' && id[i] <= '9') || id[i] == '-'))
			return 0;
	return 1;
}

/* Abre la terminal configurada (apps.terminal) con `sh -c SCRIPT rros ARG...`: así se ve el
 * progreso y se puede escribir la contraseña de sudo. Los argumentos van separados, nunca
 * dentro del script. */
static int terminal_run(const char *script, const char *a1, const char *a2, char *err, size_t en)
{
	struct model m;
	char term[64] = "foot", *words[6], copy[64], *tok, *save = NULL, full[600];
	char *argv[16];
	int n = 0, nw = 0;

	if (model_load(&m) == 0) {
		const char *t = rros_conf_get(&m.eff, "apps.terminal");

		if (t && *t && strlen(t) < sizeof term)
			snprintf(term, sizeof term, "%s", t);
		model_free(&m);
	}
	snprintf(copy, sizeof copy, "%s", term);
	for (tok = strtok_r(copy, " ", &save); tok && nw < 3; tok = strtok_r(NULL, " ", &save))
		words[nw++] = tok;
	if (nw == 0) {
		snprintf(err, en, "no hay terminal configurada (apps.terminal)");
		return -1;
	}
	snprintf(full, sizeof full, "%s; printf '\\nPulsa Enter para cerrar... '; read -r _", script);
	for (int i = 0; i < nw; i++)
		argv[n++] = words[i];
	argv[n++] = "-e";
	argv[n++] = "sh";
	argv[n++] = "-c";
	argv[n++] = full;
	argv[n++] = "rros";
	if (a1)
		argv[n++] = (char *)a1;
	if (a2)
		argv[n++] = (char *)a2;
	argv[n] = NULL;
	if (spawn_detached(argv[0], argv) != 0) {
		snprintf(err, en, "no se pudo abrir la terminal");
		return -1;
	}
	return 0;
}

/* `rros-apps install|remove ID` en una terminal */
static int run_in_terminal(const char *action, const char *id, char *err, size_t en)
{
	if (strcmp(action, "install") && strcmp(action, "remove")) {
		snprintf(err, en, "acción desconocida");
		return -1;
	}
	if (!id_valid(id)) {
		snprintf(err, en, "identificador de aplicación no válido");
		return -1;
	}
	return terminal_run("rros-apps \"$1\" \"$2\"", action, id, err, en);
}

/* Ajustes del sistema (los aplica rros-apply como root): se guardan con sudo en /etc/rros/rros.toml */
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
		return terminal_run("sudo rros-config unset --system \"$1\" && sudo rros-apply --services", key, NULL, err, en);
	}
	if (rros_user_check(key, val, err, en))
		return -1;
	return terminal_run("sudo rros-config set --system \"$1\" \"$2\" && sudo rros-apply --services", key, val, err, en);
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

static int handle(int fd, long *last_ping, int *seen_ping, int *quit)
{
	struct req r;
	char a[400], b[400], c[400], err[300];

	if (read_request(fd, &r) != 0) {
		respond(fd, 400, "text/plain", "bad request", 11, NULL);
		return 0;
	}
	if (!host_ok(r.host) || !origin_ok(r.origin)) {
		respond(fd, 403, "text/plain", "forbidden", 9, NULL);
		return 0;
	}
	if (!strcmp(r.method, "GET") && !strcmp(r.path, "/")) {
		size_t len = 0;
		char *html;

		if (!query_token_ok(r.query)) {
			respond(fd, 403, "text/plain", "falta el token (usa la URL que imprime rros-settings)", 52, NULL);
			return 0;
		}
		html = read_gui(&len);
		if (!html) {
			respond(fd, 404, "text/plain", "no se encontró index.html de la interfaz", 41, NULL);
			return 0;
		}
		respond(fd, 200, "text/html; charset=utf-8", html, len, NULL);
		free(html);
		return 0;
	}
	if (strncmp(r.path, "/api/", 5) != 0 || !token_eq(r.token)) {
		respond(fd, strncmp(r.path, "/api/", 5) ? 404 : 403, "text/plain", "no", 2, NULL);
		return 0;
	}
	*last_ping = (long)time(NULL);
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
	} else if (!strcmp(r.method, "GET") && !strcmp(r.path, "/api/templates")) {
		char *args[] = { "rros-new", "lista", "--json", NULL };

		respond_capture(fd, "rros-new", args, "las plantillas");
	} else if (strcmp(r.method, "POST") != 0) {
		respond(fd, 404, "text/plain", "no", 2, NULL);
	} else if (!strcmp(r.path, "/api/ping")) {
		*seen_ping = 1;
		respond_ok(fd, 1, "");
	} else if (!strcmp(r.path, "/api/quit")) {
		respond_ok(fd, 1, "");
		*quit = 1;
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
	} else if (!strcmp(r.path, "/api/app_run")) {
		form_get(r.body, "action", a, sizeof a);
		form_get(r.body, "id", b, sizeof b);
		if (run_in_terminal(a, b, err, sizeof err))
			respond_ok(fd, 0, err);
		else
			respond_ok(fd, 1, "");
	} else if (!strcmp(r.path, "/api/sys_set") || !strcmp(r.path, "/api/sys_unset")) {
		form_get(r.body, "key", a, sizeof a);
		form_get(r.body, "value", b, sizeof b);
		respond_ok(fd, sys_set(a, b, !strcmp(r.path, "/api/sys_unset"), err, sizeof err) == 0, err);
	} else if (!strcmp(r.path, "/api/security_level")) {
		form_get(r.body, "level", a, sizeof a);
		if (!rros_seclevel_find(a))
			respond_ok(fd, 0, "nivel desconocido");
		else
			respond_ok(fd, terminal_run("sudo rros-security nivel \"$1\"", a, NULL, err, sizeof err) == 0, err);
	} else if (!strcmp(r.path, "/api/security_port")) {
		form_get(r.body, "action", a, sizeof a);
		form_get(r.body, "port", b, sizeof b);
		if (strcmp(a, "abrir") && strcmp(a, "cerrar"))
			respond_ok(fd, 0, "acción desconocida");
		else if (!*b || strchr(b, ',') || rros_fw_ports_check(b, err, sizeof err))
			respond_ok(fd, 0, *err ? err : "indica un puerto, p. ej. 8080/tcp");
		else
			respond_ok(fd, terminal_run("sudo rros-security \"$1\" \"$2\"", a, b, err, sizeof err) == 0, err);
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
		respond(fd, 404, "text/plain", "no", 2, NULL);
	}
	return 0;
}

/* ---- ventana ---- */

static int in_path(const char *cmd)
{
	const char *path = getenv("PATH");
	char buf[700];

	if (!path)
		return 0;
	while (*path) {
		const char *colon = strchr(path, ':');
		size_t n = colon ? (size_t)(colon - path) : strlen(path);

		snprintf(buf, sizeof buf, "%.*s/%s", (int)n, path, cmd);
		if (access(buf, X_OK) == 0)
			return 1;
		if (!colon)
			break;
		path = colon + 1;
	}
	return 0;
}

int web_browser_available(void)
{
	return in_path("epiphany-browser") || in_path("chromium") || in_path("chromium-browser") ||
	       in_path("google-chrome") || in_path("falkon") || in_path("firefox") || in_path("xdg-open");
}

static int open_window(const char *url)
{
	char arg[600];
	pid_t pid;
	char *argv[8];
	int n = 0;

	if (in_path("epiphany-browser")) {
		/* el modo aplicación de Epiphany exige un perfil de "web app" con su .desktop; la instancia
		   privada abre una ventana normal con un perfil temporal y no toca el del usuario */
		argv[n++] = "epiphany-browser"; argv[n++] = "--private-instance"; argv[n++] = (char *)url;
	} else if (in_path("chromium") || in_path("chromium-browser") || in_path("google-chrome")) {
		snprintf(arg, sizeof arg, "--app=%s", url);
		argv[n++] = in_path("chromium") ? "chromium" : in_path("chromium-browser") ? "chromium-browser" : "google-chrome";
		argv[n++] = arg;
	} else if (in_path("falkon")) {
		argv[n++] = "falkon"; argv[n++] = (char *)url;
	} else if (in_path("firefox")) {
		argv[n++] = "firefox"; argv[n++] = "--new-window"; argv[n++] = (char *)url;
	} else if (in_path("xdg-open")) {
		argv[n++] = "xdg-open"; argv[n++] = (char *)url;
	} else {
		return 0;
	}
	argv[n] = NULL;
	pid = fork();
	if (pid < 0)
		return 0;
	if (pid == 0) {
		setsid();
		if (!freopen("/dev/null", "r", stdin) || !freopen("/dev/null", "w", stdout) ||
		    !freopen("/dev/null", "w", stderr))
			_exit(127);
		execvp(argv[0], argv);
		_exit(127);
	}
	return 1;
}

int run_web(int port, int open_win, const char *token, int idle_seconds)
{
	int ls = socket(AF_INET, SOCK_STREAM, 0), one = 1, quit = 0, seen_ping = 0;
	struct sockaddr_in sa;
	socklen_t sl = sizeof sa;
	long last_ping, last_req;
	char url[200];

	signal(SIGPIPE, SIG_IGN);
	if (ls < 0)
		return 1;
	setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
	memset(&sa, 0, sizeof sa);
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	sa.sin_port = htons((unsigned short)port);
	if (bind(ls, (struct sockaddr *)&sa, sizeof sa) != 0 || listen(ls, 8) != 0 ||
	    getsockname(ls, (struct sockaddr *)&sa, &sl) != 0) {
		perror("rros-settings");
		return 1;
	}
	port_g = ntohs(sa.sin_port);
	if (token && *token) {
		snprintf(token_g, sizeof token_g, "%s", token);
	} else {
		unsigned char rnd[16];
		FILE *f = fopen("/dev/urandom", "r");

		if (!f || fread(rnd, 1, sizeof rnd, f) != sizeof rnd) {
			fputs("rros-settings: no se pudo generar el token\n", stderr);
			return 1;
		}
		fclose(f);
		for (size_t i = 0; i < sizeof rnd; i++)
			snprintf(token_g + i * 2, 3, "%02x", rnd[i]);
	}
	snprintf(url, sizeof url, "http://127.0.0.1:%d/?t=%s%s%s", port_g, token_g, *start_page_g ? "#" : "", start_page_g);
	printf("RROS Ajustes en %s\n", url);
	fflush(stdout);
	last_ping = last_req = (long)time(NULL);
	if (open_win && !open_window(url))
		fputs("rros-settings: no hay navegador; abre la dirección de arriba, o usa `rros-settings tui`\n", stderr);

	while (!quit) {
		struct pollfd pf = { ls, POLLIN, 0 };
		long now = (long)time(NULL);
		int cfd;

		if (poll(&pf, 1, 1000) > 0 && (pf.revents & POLLIN)) {
			cfd = accept(ls, NULL, NULL);
			if (cfd >= 0) {
				last_req = (long)time(NULL);
				handle(cfd, &last_ping, &seen_ping, &quit);
				close(cfd);
			}
		}
		if (idle_seconds > 0 && now - last_req > idle_seconds)
			break;
		if (seen_ping && now - last_ping > 25)
			break;
	}
	close(ls);
	return 0;
}
