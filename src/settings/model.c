#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "rros_hw.h"
#include "rros_profile.h"
#include "rros_userconf.h"
#include "rros_version.h"
#include "settings.h"
#include "theme.h"

static void load_opt(struct rros_conf *c, const char *path)
{
	int rc = rros_conf_load(c, path, NULL);

	if (rc) {
		rros_conf_free(c);
		rros_conf_init(c);
	}
}

int model_load(struct model *m)
{
	char user[1024];

	memset(m, 0, sizeof *m);
	rros_conf_init(&m->eff);
	rros_conf_init(&m->prof);
	rros_conf_init(&m->sys);
	rros_conf_init(&m->user);
	rros_read_active_profile(m->profile, sizeof m->profile);
	if (rros_load_profile(&m->prof, m->profile, NULL) != 0)
		return -1;
	load_opt(&m->sys, rros_system_conf());
	if (rros_user_conf(user, sizeof user) == 0)
		load_opt(&m->user, user);
	return rros_load_effective(&m->eff, m->profile) == 0 ? 0 : -1;
}

void model_free(struct model *m)
{
	rros_conf_free(&m->eff);
	rros_conf_free(&m->prof);
	rros_conf_free(&m->sys);
	rros_conf_free(&m->user);
}

const char *model_source(const struct model *m, const char *key)
{
	if (rros_conf_get(&m->user, key))
		return "user";
	if (rros_conf_get(&m->sys, key))
		return "system";
	return "profile";
}

/* ---- categorías ---- */

static const struct { const char *id, *label; } cats[] = {
	{ "ui", "Interfaz" }, { "panel", "Panel" }, { "term", "Terminal" }, { "display", "Pantalla" },
	{ "input", "Entrada" }, { "audio", "Sonido" }, { "power", "Energía" }, { "net", "Red" },
	{ "system", "Sistema" }, { "services", "Servicios" }, { "locale", "Región" },
	{ "privacy", "Privacidad" }, { "boot", "Arranque" },
};

size_t model_categories(char names[][16], size_t max)
{
	size_t n = 0;

	for (size_t i = 0; i < sizeof cats / sizeof *cats && n < max; i++)
		snprintf(names[n++], 16, "%s", cats[i].id);
	return n;
}

const char *model_category_label(const char *cat)
{
	for (size_t i = 0; i < sizeof cats / sizeof *cats; i++)
		if (!strcmp(cats[i].id, cat))
			return cats[i].label;
	return cat;
}

/* ---- sbuf ---- */

void sb_free(struct sbuf *b)
{
	free(b->p);
	b->p = NULL;
	b->n = b->cap = 0;
}

static int sb_grow(struct sbuf *b, size_t extra)
{
	size_t need = b->n + extra + 1;

	if (need > b->cap) {
		size_t nc = b->cap ? b->cap : 4096;
		char *np;

		while (nc < need)
			nc *= 2;
		np = realloc(b->p, nc);
		if (!np)
			return -1;
		b->p = np;
		b->cap = nc;
	}
	return 0;
}

int sb_put(struct sbuf *b, const char *s)
{
	size_t l = strlen(s);

	if (sb_grow(b, l))
		return -1;
	memcpy(b->p + b->n, s, l + 1);
	b->n += l;
	return 0;
}

int sb_printf(struct sbuf *b, const char *fmt, ...)
{
	va_list ap, ap2;
	int l;

	va_start(ap, fmt);
	va_copy(ap2, ap);
	l = vsnprintf(NULL, 0, fmt, ap);
	va_end(ap);
	if (l < 0 || sb_grow(b, (size_t)l)) {
		va_end(ap2);
		return -1;
	}
	vsnprintf(b->p + b->n, (size_t)l + 1, fmt, ap2);
	va_end(ap2);
	b->n += (size_t)l;
	return 0;
}

int sb_json(struct sbuf *b, const char *s)
{
	if (sb_put(b, "\""))
		return -1;
	for (; *s; s++) {
		unsigned char c = (unsigned char)*s;
		int rc;

		if (c == '"' || c == '\\')
			rc = sb_printf(b, "\\%c", c);
		else if (c == '\n')
			rc = sb_put(b, "\\n");
		else if (c == '\t')
			rc = sb_put(b, "\\t");
		else if (c < 0x20)
			rc = sb_printf(b, "\\u%04x", c);
		else
			rc = sb_printf(b, "%c", c);
		if (rc)
			return -1;
	}
	return sb_put(b, "\"");
}

/* ---- estado en JSON ---- */

static void put_palette(struct sbuf *b, const struct rros_palette *p)
{
	sb_printf(b, "{\"bg\":\"%s\",\"surface\":\"%s\",\"text\":\"%s\",\"muted\":\"%s\",\"border\":\"%s\","
		     "\"red\":\"%s\",\"green\":\"%s\",\"yellow\":\"%s\",\"blue\":\"%s\",\"magenta\":\"%s\","
		     "\"cyan\":\"%s\",\"accent\":\"%s\"}",
		  p->bg, p->surface, p->text, p->muted, p->border, p->red, p->green, p->yellow, p->blue,
		  p->magenta, p->cyan, p->accent);
}

static void put_choices(struct sbuf *b, const char *choices)
{
	const char *p = choices;

	sb_put(b, "[");
	while (*p) {
		const char *bar = strchr(p, '|');
		size_t n = bar ? (size_t)(bar - p) : strlen(p);
		char tmp[64];

		snprintf(tmp, sizeof tmp, "%.*s", (int)n, p);
		sb_json(b, tmp);
		if (!bar)
			break;
		sb_put(b, ",");
		p = bar + 1;
	}
	sb_put(b, "]");
}

static const char *category_of(const char *key, char *buf, size_t n)
{
	const char *dot = strchr(key, '.');
	size_t l = dot ? (size_t)(dot - key) : strlen(key);

	snprintf(buf, n, "%.*s", (int)l, key);
	return buf;
}

static void put_dynamic(struct sbuf *b, const struct model *m, const char *prefix, const char *name, int last)
{
	size_t pl = strlen(prefix);
	int first = 1;

	sb_printf(b, "\"%s\":[", name);
	for (size_t i = 0; i < m->eff.n; i++) {
		const struct rros_kv *kv = &m->eff.items[i];

		if (strncmp(kv->key, prefix, pl))
			continue;
		sb_put(b, first ? "{\"id\":" : ",{\"id\":");
		sb_json(b, kv->key + pl);
		sb_put(b, ",\"value\":");
		sb_json(b, kv->val);
		sb_printf(b, ",\"source\":\"%s\"}", model_source(m, kv->key));
		first = 0;
	}
	sb_put(b, last ? "]" : "],");
}

int model_state_json(const struct model *m, struct sbuf *o)
{
	struct rros_theme th;
	struct rros_palette cur;
	struct rros_theme_info themes[64];
	size_t nth, first = 1;
	char cat[16], user[1024];
	int firstkey = 1;

	rros_theme_from_conf(&th, &m->eff);
	rros_theme_palette(&th, &cur);
	sb_printf(o, "{\"version\":\"%s\",\"codename\":\"%s\",\"profile\":", RROS_VERSION, RROS_CODENAME);
	sb_json(o, m->profile);

	/* categorías */
	{
		char names[16][16];
		size_t n = model_categories(names, 16);

		sb_put(o, ",\"categories\":[");
		for (size_t i = 0; i < n; i++) {
			sb_printf(o, "%s{\"id\":\"%s\",\"label\":", i ? "," : "", names[i]);
			sb_json(o, model_category_label(names[i]));
			sb_put(o, "}");
		}
		sb_put(o, "]");
	}

	/* claves estáticas */
	sb_put(o, ",\"keys\":[");
	for (size_t i = 0; i < rros_schema_len; i++) {
		const struct rros_key *k = &rros_schema[i];
		const char *v;

		if (k->profile_only)
			continue;
		v = rros_conf_get(&m->eff, k->key);
		sb_put(o, firstkey ? "{\"key\":" : ",{\"key\":");
		firstkey = 0;
		sb_json(o, k->key);
		sb_printf(o, ",\"category\":\"%s\",\"type\":", category_of(k->key, cat, sizeof cat));
		sb_json(o, k->type == RROS_T_INT ? "int" : k->type == RROS_T_BOOL ? "bool" :
			   k->type == RROS_T_ENUM ? "enum" : "str");
		if (k->type == RROS_T_INT)
			sb_printf(o, ",\"min\":%ld,\"max\":%ld", k->min, k->max);
		if (k->type == RROS_T_ENUM) {
			sb_put(o, ",\"choices\":");
			put_choices(o, k->choices);
		}
		sb_put(o, ",\"desc\":");
		sb_json(o, k->desc);
		sb_put(o, ",\"value\":");
		sb_json(o, v ? v : "");
		sb_printf(o, ",\"source\":\"%s\"}", model_source(m, k->key));
	}
	sb_put(o, "]");

	/* claves dinámicas */
	sb_put(o, ",\"dynamic\":{");
	put_dynamic(o, m, "keybind.", "keybind", 0);
	put_dynamic(o, m, "rule.", "rule", 0);
	put_dynamic(o, m, "autostart.", "autostart", 1);
	sb_put(o, "},\"dynamic_help\":{");
	sb_put(o, "\"keybind\":"); sb_json(o, rros_family_desc(RROS_FAM_KEYBIND));
	sb_put(o, ",\"rule\":"); sb_json(o, rros_family_desc(RROS_FAM_RULE));
	sb_put(o, ",\"autostart\":"); sb_json(o, rros_family_desc(RROS_FAM_AUTOSTART));
	sb_put(o, "}");

	/* temas */
	nth = rros_theme_list(themes, 64);
	sb_put(o, ",\"themes\":[");
	for (size_t i = 0; i < nth; i++) {
		struct rros_theme t = th;
		struct rros_palette pd, pl;

		snprintf(t.theme, sizeof t.theme, "%.39s", themes[i].name);
		t.accent[0] = '\0';
		t.dark = 1;
		rros_theme_palette(&t, &pd);
		t.dark = 0;
		rros_theme_palette(&t, &pl);
		sb_put(o, first ? "{\"name\":" : ",{\"name\":");
		first = 0;
		sb_json(o, themes[i].name);
		sb_printf(o, ",\"builtin\":%s,\"dark\":", themes[i].builtin ? "true" : "false");
		put_palette(o, &pd);
		sb_put(o, ",\"light\":");
		put_palette(o, &pl);
		sb_put(o, "}");
	}
	sb_put(o, "]");
	sb_printf(o, ",\"current\":{\"theme\":");
	sb_json(o, th.theme);
	sb_printf(o, ",\"scheme\":\"%s\",\"accent_user\":%s,\"palette\":", th.dark ? "dark" : "light",
		  th.accent[0] ? "true" : "false");
	put_palette(o, &cur);
	sb_put(o, "}");

	/* instantáneas y perfiles */
	{
		char *buf = NULL;
		size_t len = 0;
		FILE *f = open_memstream(&buf, &len);
		char *line, *save = NULL;
		int firstsnap = 1;

		if (f) {
			rros_snapshot_list(f);
			fclose(f);
		}
		sb_put(o, ",\"snapshots\":[");
		for (line = strtok_r(buf, "\n", &save); line; line = strtok_r(NULL, "\n", &save)) {
			sb_put(o, firstsnap ? "" : ",");
			sb_json(o, line);
			firstsnap = 0;
		}
		sb_put(o, "]");
		free(buf);
	}
	{
		DIR *d = opendir(rros_profile_dir());
		struct dirent *e;
		int firstp = 1;

		sb_put(o, ",\"profiles\":[");
		while (d && (e = readdir(d))) {
			size_t l = strlen(e->d_name);

			if (l > 5 && l - 5 <= 32 && !strcmp(e->d_name + l - 5, ".toml")) {
				char nm[40];

				memcpy(nm, e->d_name, l - 5);
				nm[l - 5] = '\0';
				sb_put(o, firstp ? "" : ",");
				sb_json(o, nm);
				firstp = 0;
			}
		}
		if (d)
			closedir(d);
		sb_put(o, "]");
	}
	{
		struct rros_hw hw;

		if (rros_hw_detect(&hw, NULL) == 0)
			sb_printf(o, ",\"hw\":{\"ram_mb\":%ld,\"cpus\":%d,\"gpu\":\"%s\",\"recommended\":\"%s\"}",
				  hw.ram_mb, hw.cpus, rros_gpu_name(hw.gpu), rros_hw_pick_profile(&hw));
	}
	if (rros_user_conf(user, sizeof user) == 0) {
		sb_put(o, ",\"user_conf\":");
		sb_json(o, user);
	}
	return sb_put(o, "}");
}

/* ---- procesos ---- */

static char tool_dir[512];

void tool_dir_from_argv0(const char *argv0)
{
	const char *slash = strrchr(argv0, '/');

	if (slash && (size_t)(slash - argv0) < sizeof tool_dir) {
		memcpy(tool_dir, argv0, (size_t)(slash - argv0));
		tool_dir[slash - argv0] = '\0';
	}
}

int run_tool(const char *tool, char *const argv[], int quiet)
{
	pid_t pid;
	int st;

	fflush(stdout);
	fflush(stderr);
	pid = fork();
	if (pid < 0)
		return -1;
	if (pid == 0) {
		char path[640];

		if (quiet) {
			if (!freopen("/dev/null", "w", stdout) || !freopen("/dev/null", "w", stderr))
				_exit(127);
		}
		if (tool_dir[0]) {
			snprintf(path, sizeof path, "%s/%s", tool_dir, tool);
			if (access(path, X_OK) == 0)
				execv(path, argv);
		}
		execvp(tool, argv);
		_exit(127);
	}
	if (waitpid(pid, &st, 0) < 0)
		return -1;
	return WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}
