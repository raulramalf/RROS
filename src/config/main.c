/* rros-config: personalización por capas (perfil < sistema < usuario). */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rros_conf.h"
#include "rros_profile.h"
#include "rros_schema.h"

static void usage(void)
{
	fputs("uso: rros-config [-p PERFIL] [-f ARCHIVO] COMANDO\n"
	      "  get CLAVE              valor efectivo\n"
	      "  list                   todas las claves efectivas y su capa\n"
	      "  set [--system] CLAVE VALOR\n"
	      "  unset [--system] CLAVE\n"
	      "  validate [--profile-file] [ARCHIVO]\n"
	      "  schema [--markdown]    claves disponibles\n"
	      "  profile                perfil activo\n"
	      "  -f ARCHIVO             get/list leen solo ese archivo (para scripts)\n", stderr);
}

enum layer { L_PROFILE, L_SYSTEM, L_USER };
static const char *layer_name[] = { "perfil", "sistema", "usuario" };

struct layers {
	struct rros_conf l[3];
	struct rros_conf merged;
};

static int load_optional(struct rros_conf *c, const char *path)
{
	int line = 0, rc = rros_conf_load(c, path, &line);

	if (rc == -1 && errno == ENOENT)
		return 0;
	if (rc == -1) {
		fprintf(stderr, "rros-config: %s: %s\n", path, strerror(errno));
		return -1;
	}
	if (rc != 0) {
		fprintf(stderr, "rros-config: %s:%d: error de sintaxis\n", path, line);
		return -1;
	}
	return 0;
}

static int load_layers(struct layers *ls, const char *profile, const char *only_file)
{
	char user[1024];
	int line = 0;

	for (int i = 0; i < 3; i++)
		rros_conf_init(&ls->l[i]);
	rros_conf_init(&ls->merged);
	if (only_file) {
		int rc = rros_conf_load(&ls->l[L_PROFILE], only_file, &line);

		if (rc) {
			fprintf(stderr, "rros-config: no se pudo leer %s\n", only_file);
			return -1;
		}
	} else {
		int rc = rros_load_profile(&ls->l[L_PROFILE], profile, &line);

		if (rc == -1 || rc == -2) {
			fprintf(stderr, "rros-config: perfil '%s' no encontrado en %s\n",
				profile, rros_profile_dir());
			return -1;
		}
		if (rc) {
			fprintf(stderr, "rros-config: perfil '%s':%d: error de sintaxis\n", profile, line);
			return -1;
		}
		if (load_optional(&ls->l[L_SYSTEM], rros_system_conf()))
			return -1;
		if (rros_user_conf(user, sizeof user) == 0 &&
		    load_optional(&ls->l[L_USER], user))
			return -1;
	}
	for (int i = 0; i < 3; i++)
		for (size_t j = 0; j < ls->l[i].n; j++)
			if (rros_conf_set(&ls->merged, ls->l[i].items[j].key,
					  ls->l[i].items[j].val, ls->l[i].items[j].quoted))
				return -1;
	return 0;
}

static void free_layers(struct layers *ls)
{
	for (int i = 0; i < 3; i++)
		rros_conf_free(&ls->l[i]);
	rros_conf_free(&ls->merged);
}

/* Valida pares clave/valor de una configuración. Devuelve nº de errores. */
static int validate_conf(const struct rros_conf *c, const char *name, int profile_file)
{
	char err[256];
	int bad = 0;

	for (size_t i = 0; i < c->n; i++) {
		const struct rros_kv *kv = &c->items[i];
		const struct rros_key *k = rros_schema_find(kv->key);

		if (!k) {
			fprintf(stderr, "%s: clave desconocida '%s'\n", name, kv->key);
			bad++;
		} else if (k->profile_only && !profile_file) {
			fprintf(stderr, "%s: '%s' solo es válida en archivos de perfil\n", name, kv->key);
			bad++;
		} else if (rros_schema_check(k, kv->val, kv->quoted, err, sizeof err)) {
			fprintf(stderr, "%s: %s\n", name, err);
			bad++;
		}
	}
	return bad;
}

static int cmd_validate(int argc, char **argv)
{
	int profile_file = 0, bad = 0;
	const char *file = NULL;
	char user[1024];

	for (int i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "--profile-file"))
			profile_file = 1;
		else if (!file)
			file = argv[i];
		else
			return 2;
	}
	if (!file) {
		struct rros_conf c;
		const char *paths[2] = { rros_system_conf(), user };

		if (rros_user_conf(user, sizeof user) != 0)
			paths[1] = NULL;
		for (int i = 0; i < 2; i++) {
			if (!paths[i])
				continue;
			rros_conf_init(&c);
			if (load_optional(&c, paths[i]))
				bad++;
			else
				bad += validate_conf(&c, paths[i], 0);
			rros_conf_free(&c);
		}
	} else {
		struct rros_conf c;
		int line = 0, rc;

		rros_conf_init(&c);
		rc = rros_conf_load(&c, file, &line);
		if (rc == -1) {
			fprintf(stderr, "rros-config: %s: %s\n", file, strerror(errno));
			bad++;
		} else if (rc) {
			fprintf(stderr, "%s:%d: error de sintaxis\n", file, line);
			bad++;
		} else {
			bad += validate_conf(&c, file, profile_file);
		}
		rros_conf_free(&c);
	}
	if (bad)
		fprintf(stderr, "%d error(es)\n", bad);
	return bad ? 1 : 0;
}

static int cmd_schema(int markdown)
{
	if (markdown) {
		puts("# Personalización\n");
		puts("Generado con `rros-config schema --markdown`. Capas: perfil < sistema < usuario.\n");
		puts("| Clave | Tipo | Valores | Descripción |");
		puts("|---|---|---|---|");
	}
	for (size_t i = 0; i < rros_schema_len; i++) {
		const struct rros_key *k = &rros_schema[i];
		char vals[128] = "";

		if (k->type == RROS_T_INT)
			snprintf(vals, sizeof vals, "%ld – %ld", k->min, k->max);
		else if (k->type == RROS_T_ENUM)
			snprintf(vals, sizeof vals, "%s", k->choices);
		else if (k->type == RROS_T_BOOL)
			snprintf(vals, sizeof vals, "true / false");
		if (k->profile_only && markdown)
			continue;
		if (markdown)
			printf("| `%s` | %s | %s | %s |\n", k->key, rros_type_name(k->type), vals, k->desc);
		else
			printf("%-28s %-9s %-34s %s%s\n", k->key, rros_type_name(k->type), vals,
			       k->desc, k->profile_only ? " [solo perfil]" : "");
	}
	return 0;
}

static int cmd_set(int argc, char **argv, int unset)
{
	int system = 0, need = unset ? 1 : 2, a = 0;
	const struct rros_key *k;
	struct rros_conf c;
	char path[1024], err[256];
	int rc, line = 0;
	const char *key, *val = NULL;

	if (argc > 0 && !strcmp(argv[0], "--system")) {
		system = 1;
		a = 1;
	}
	if (argc - a != need)
		return 2;
	key = argv[a];
	if (!unset)
		val = argv[a + 1];
	k = rros_schema_find(key);
	if (!k || k->profile_only) {
		fprintf(stderr, "rros-config: clave '%s' %s\n", key,
			k ? "solo es válida en perfiles" : "desconocida (ver `rros-config schema`)");
		return 1;
	}
	if (!unset && rros_schema_check(k, val, k->type == RROS_T_STR || k->type == RROS_T_ENUM,
					err, sizeof err)) {
		fprintf(stderr, "rros-config: %s\n", err);
		return 1;
	}
	if (system)
		snprintf(path, sizeof path, "%s", rros_system_conf());
	else if (rros_user_conf(path, sizeof path)) {
		fputs("rros-config: no se pudo determinar el archivo de usuario\n", stderr);
		return 1;
	}
	rros_conf_init(&c);
	rc = rros_conf_load(&c, path, &line);
	if (rc && !(rc == -1 && errno == ENOENT)) {
		fprintf(stderr, "rros-config: %s: archivo ilegible o con errores\n", path);
		rros_conf_free(&c);
		return 1;
	}
	if (unset)
		rros_conf_unset(&c, key);
	else if (rros_conf_set(&c, key, val, k->type == RROS_T_STR || k->type == RROS_T_ENUM)) {
		rros_conf_free(&c);
		return 1;
	}
	if (rros_mkdir_parents(path) || rros_conf_save(&c, path)) {
		fprintf(stderr, "rros-config: no se pudo escribir %s: %s\n", path, strerror(errno));
		rros_conf_free(&c);
		return 1;
	}
	rros_conf_free(&c);
	return 0;
}

int main(int argc, char **argv)
{
	char profile[64] = "";
	const char *only_file = NULL;
	int i = 1;
	struct layers ls;
	const char *cmd;
	int rc = 0;

	for (; i < argc && argv[i][0] == '-'; i++) {
		if (!strcmp(argv[i], "-p") && i + 1 < argc)
			snprintf(profile, sizeof profile, "%s", argv[++i]);
		else if (!strcmp(argv[i], "-f") && i + 1 < argc)
			only_file = argv[++i];
		else
			break;
	}
	if (i >= argc) {
		usage();
		return 2;
	}
	cmd = argv[i++];
	if (!*profile)
		rros_read_active_profile(profile, sizeof profile);

	if (!strcmp(cmd, "validate"))
		rc = cmd_validate(argc - i, argv + i);
	else if (!strcmp(cmd, "schema"))
		rc = cmd_schema(i < argc && !strcmp(argv[i], "--markdown"));
	else if (!strcmp(cmd, "set"))
		rc = cmd_set(argc - i, argv + i, 0);
	else if (!strcmp(cmd, "unset"))
		rc = cmd_set(argc - i, argv + i, 1);
	else if (!strcmp(cmd, "profile"))
		puts(profile);
	else if (!strcmp(cmd, "get") && i + 1 == argc) {
		const char *v;

		if (load_layers(&ls, profile, only_file))
			rc = 1;
		else if ((v = rros_conf_get(&ls.merged, argv[i])))
			puts(v);
		else
			rc = 1;
		free_layers(&ls);
	} else if (!strcmp(cmd, "list") && i == argc) {
		if (load_layers(&ls, profile, only_file)) {
			rc = 1;
		} else {
			rros_conf_sort(&ls.merged);
			for (size_t j = 0; j < ls.merged.n; j++) {
				const struct rros_kv *kv = &ls.merged.items[j];
				int layer = L_PROFILE;

				if (rros_conf_get(&ls.l[L_USER], kv->key))
					layer = L_USER;
				else if (rros_conf_get(&ls.l[L_SYSTEM], kv->key))
					layer = L_SYSTEM;
				printf("%s = %s%s%s  (%s)\n", kv->key, kv->quoted ? "\"" : "", kv->val,
				       kv->quoted ? "\"" : "", layer_name[layer]);
			}
		}
		free_layers(&ls);
	} else {
		usage();
		return 2;
	}
	if (rc == 2)
		usage();
	return rc;
}
