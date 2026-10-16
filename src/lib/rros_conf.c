#include "rros_conf.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SECTION_MAX 128
#define KEY_MAX 200

static char *dup_n(const char *s, size_t n)
{
	char *p = malloc(n + 1);

	if (!p)
		return NULL;
	memcpy(p, s, n);
	p[n] = '\0';
	return p;
}

void rros_conf_init(struct rros_conf *c)
{
	c->items = NULL;
	c->n = c->cap = 0;
}

void rros_conf_free(struct rros_conf *c)
{
	for (size_t i = 0; i < c->n; i++) {
		free(c->items[i].key);
		free(c->items[i].val);
	}
	free(c->items);
	rros_conf_init(c);
}

static struct rros_kv *find(const struct rros_conf *c, const char *key)
{
	for (size_t i = 0; i < c->n; i++)
		if (strcmp(c->items[i].key, key) == 0)
			return &c->items[i];
	return NULL;
}

const char *rros_conf_get(const struct rros_conf *c, const char *key)
{
	const struct rros_kv *kv = find(c, key);

	return kv ? kv->val : NULL;
}

long rros_conf_get_long(const struct rros_conf *c, const char *key, long def)
{
	const char *v = rros_conf_get(c, key);
	char *end;
	long x;

	if (!v || !*v)
		return def;
	x = strtol(v, &end, 10);
	return *end ? def : x;
}

int rros_conf_get_bool(const struct rros_conf *c, const char *key, int def)
{
	const char *v = rros_conf_get(c, key);

	if (!v)
		return def;
	if (strcmp(v, "true") == 0)
		return 1;
	if (strcmp(v, "false") == 0)
		return 0;
	return def;
}

int rros_conf_set(struct rros_conf *c, const char *key, const char *val, int quoted)
{
	struct rros_kv *kv = find(c, key);
	char *v = dup_n(val, strlen(val));

	if (!v)
		return -1;
	if (kv) {
		free(kv->val);
		kv->val = v;
		kv->quoted = quoted;
		return 0;
	}
	if (c->n == c->cap) {
		size_t ncap = c->cap ? c->cap * 2 : 16;
		struct rros_kv *ni = realloc(c->items, ncap * sizeof *ni);

		if (!ni) {
			free(v);
			return -1;
		}
		c->items = ni;
		c->cap = ncap;
	}
	c->items[c->n].key = dup_n(key, strlen(key));
	if (!c->items[c->n].key) {
		free(v);
		return -1;
	}
	c->items[c->n].val = v;
	c->items[c->n].quoted = quoted;
	c->n++;
	return 0;
}

int rros_conf_unset(struct rros_conf *c, const char *key)
{
	for (size_t i = 0; i < c->n; i++) {
		if (strcmp(c->items[i].key, key) == 0) {
			free(c->items[i].key);
			free(c->items[i].val);
			c->items[i] = c->items[--c->n];
			return 1;
		}
	}
	return 0;
}

/* ---- parser ---- */

static char *trim(char *s)
{
	size_t n;

	while (isspace((unsigned char)*s))
		s++;
	n = strlen(s);
	while (n && isspace((unsigned char)s[n - 1]))
		s[--n] = '\0';
	return s;
}

static void strip_comment(char *s)
{
	int in_quote = 0;

	for (; *s; s++) {
		if (in_quote) {
			if (*s == '\\' && s[1]) {
				s++;
				continue;
			}
			if (*s == '"')
				in_quote = 0;
		} else if (*s == '"') {
			in_quote = 1;
		} else if (*s == '#') {
			*s = '\0';
			return;
		}
	}
}

static int key_ok(const char *k, int allow_dot)
{
	if (!*k)
		return 0;
	for (; *k; k++) {
		if (isalnum((unsigned char)*k) || *k == '_' || *k == '-')
			continue;
		if (allow_dot && *k == '.')
			continue;
		return 0;
	}
	return 1;
}

static int parse_string(struct rros_conf *c, const char *full, char *val)
{
	char *out = malloc(strlen(val) + 1);
	size_t o = 0;
	char *p = val + 1;
	int closed = 0, rc;

	if (!out)
		return -3;
	for (; *p; p++) {
		if (*p == '\\') {
			p++;
			if (*p == 'n') {
				out[o++] = '\n';
			} else if (*p == '"' || *p == '\\') {
				out[o++] = *p;
			} else {
				free(out);
				return -2;
			}
		} else if (*p == '"') {
			closed = 1;
			p++;
			break;
		} else {
			out[o++] = *p;
		}
	}
	if (!closed || *trim(p)) {
		free(out);
		return -2;
	}
	out[o] = '\0';
	rc = rros_conf_set(c, full, out, 1);
	free(out);
	return rc ? -3 : 0;
}

static int parse_line(struct rros_conf *c, char *line, char *section)
{
	char full[SECTION_MAX + KEY_MAX + 2];
	char *s, *eq, *key, *val;
	size_t n;

	strip_comment(line);
	s = trim(line);
	if (!*s)
		return 0;
	if (*s == '[') {
		n = strlen(s);
		if (s[n - 1] != ']')
			return -2;
		s[n - 1] = '\0';
		s = trim(s + 1);
		if (!key_ok(s, 1) || strlen(s) >= SECTION_MAX)
			return -2;
		strcpy(section, s);
		return 0;
	}
	eq = strchr(s, '=');
	if (!eq)
		return -2;
	*eq = '\0';
	key = trim(s);
	val = trim(eq + 1);
	if (!key_ok(key, 0) || strlen(key) > KEY_MAX || !*val)
		return -2;
	if (*section)
		snprintf(full, sizeof full, "%s.%s", section, key);
	else
		snprintf(full, sizeof full, "%s", key);
	if (*val == '"')
		return parse_string(c, full, val);
	for (char *p = val; *p; p++)
		if (isspace((unsigned char)*p) || *p == '"')
			return -2;
	return rros_conf_set(c, full, val, 0) ? -3 : 0;
}

int rros_conf_parse(struct rros_conf *c, const char *text, int *errline)
{
	char section[SECTION_MAX] = "";
	const char *p = text;
	int ln = 0;

	while (*p) {
		const char *nl = strchr(p, '\n');
		size_t n = nl ? (size_t)(nl - p) : strlen(p);
		char *line = dup_n(p, n);
		int rc;

		if (!line)
			return -3;
		ln++;
		rc = parse_line(c, line, section);
		free(line);
		if (rc) {
			if (errline)
				*errline = ln;
			return rc;
		}
		p += n;
		if (nl)
			p++;
	}
	return 0;
}

int rros_conf_load(struct rros_conf *c, const char *path, int *errline)
{
	FILE *f = fopen(path, "r");
	char *buf = NULL, *nb;
	size_t len = 0, cap = 0, r;
	int rc;

	if (!f)
		return -1;
	for (;;) {
		if (len + 4096 + 1 > cap) {
			cap = cap ? cap * 2 : 8192;
			nb = realloc(buf, cap);
			if (!nb) {
				free(buf);
				fclose(f);
				return -3;
			}
			buf = nb;
		}
		r = fread(buf + len, 1, 4096, f);
		len += r;
		if (r < 4096)
			break;
	}
	fclose(f);
	if (!buf)
		return 0;
	buf[len] = '\0';
	if (strlen(buf) != len) { /* byte NUL embebido */
		free(buf);
		if (errline)
			*errline = 1;
		return -2;
	}
	rc = rros_conf_parse(c, buf, errline);
	free(buf);
	return rc;
}

/* ---- orden y escritura ---- */

static size_t section_len(const char *k)
{
	const char *d = strrchr(k, '.');

	return d ? (size_t)(d - k) : 0;
}

static int kv_cmp(const void *a, const void *b)
{
	const struct rros_kv *x = a, *y = b;
	size_t sx = section_len(x->key), sy = section_len(y->key);
	size_t m = sx < sy ? sx : sy;
	int r;

	if ((sx == 0) != (sy == 0))
		return sx == 0 ? -1 : 1;
	r = memcmp(x->key, y->key, m);
	if (r)
		return r;
	if (sx != sy)
		return sx < sy ? -1 : 1;
	return strcmp(x->key + (sx ? sx + 1 : 0), y->key + (sy ? sy + 1 : 0));
}

void rros_conf_sort(struct rros_conf *c)
{
	if (c->n > 1)
		qsort(c->items, c->n, sizeof *c->items, kv_cmp);
}

static void put_value(FILE *f, const struct rros_kv *kv)
{
	if (!kv->quoted) {
		fputs(kv->val, f);
		return;
	}
	fputc('"', f);
	for (const char *p = kv->val; *p; p++) {
		if (*p == '"' || *p == '\\')
			fprintf(f, "\\%c", *p);
		else if (*p == '\n')
			fputs("\\n", f);
		else
			fputc(*p, f);
	}
	fputc('"', f);
}

int rros_conf_save(struct rros_conf *c, const char *path)
{
	size_t pl = strlen(path);
	char *tmp = malloc(pl + 5);
	char cur[SECTION_MAX] = "";
	int wrote = 0, rc = 0;
	FILE *f;

	if (!tmp)
		return -3;
	memcpy(tmp, path, pl);
	strcpy(tmp + pl, ".tmp");
	rros_conf_sort(c);
	f = fopen(tmp, "w");
	if (!f) {
		free(tmp);
		return -1;
	}
	for (size_t i = 0; i < c->n; i++) {
		const struct rros_kv *kv = &c->items[i];
		size_t sl = section_len(kv->key);
		const char *k = kv->key;

		if (sl >= SECTION_MAX) {
			rc = -2;
			break;
		}
		if (sl) {
			if (strlen(cur) != sl || memcmp(cur, k, sl) != 0) {
				fprintf(f, "%s[%.*s]\n", wrote ? "\n" : "", (int)sl, k);
				memcpy(cur, k, sl);
				cur[sl] = '\0';
			}
			k += sl + 1;
		}
		fprintf(f, "%s = ", k);
		put_value(f, kv);
		fputc('\n', f);
		wrote = 1;
	}
	if (fclose(f) != 0)
		rc = rc ? rc : -1;
	if (rc == 0 && rename(tmp, path) != 0)
		rc = -1;
	if (rc != 0)
		remove(tmp);
	free(tmp);
	return rc;
}
