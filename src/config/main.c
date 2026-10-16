/* rros-config: personalización por capas (perfil < sistema < usuario). */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rros_conf.h"
#include "rros_profile.h"
#include "rros_schema.h"
#include "rros_userconf.h"

static void usage(FILE *out)
{
	fputs("uso: rros-config [-p PERFIL] [-f ARCHIVO] COMANDO\n"
	      "\n"
	      "Leer\n"
	      "  get CLAVE                 valor efectivo           rros-config get ui.gap\n"
	      "  list                      todo, con su capa        rros-config list\n"
	      "  schema [--markdown]       claves disponibles\n"
	      "  profile                   perfil activo\n"
	      "Cambiar\n"
	      "  set [--system] CLAVE VALOR   rros-config set ui.accent '#22d3ee'\n"
	      "  unset [--system] CLAVE       vuelve al valor del perfil\n"
	      "  reset --yes               borra todos tus ajustes (guarda copia .bak)\n"
	      "Copias\n"
	      "  export                    imprime tus ajustes      rros-config export > mis-ajustes.toml\n"
	      "  import ARCHIVO            reemplaza tus ajustes si son válidos\n"
	      "  snapshot save|restore|delete NOMBRE | list\n"
	      "Comprobar\n"
	      "  validate [--profile-file] [ARCHIVO]\n"
	      "  -f ARCHIVO                get/list leen solo ese archivo (scripts)\n"
	      "\n"
	      "Claves dinámicas (tú eliges el identificador):\n"
	      "  keybind.<id>    rros-config set keybind.nav 'Mod+Shift+b exec firefox'\n"
	      "  rule.<id>       rros-config set rule.ff 'app_id=firefox workspace 2'\n"
	      "  autostart.<id>  rros-config set autostart.sync 'syncthing -no-browser'\n", out);
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
		if (rros_conf_load(&ls->l[L_PROFILE], only_file, &line)) {
			fprintf(stderr, "rros-config: no se pudo leer %s\n", only_file);
			return -1;
		}
	} else {
		int rc = rros_load_profile(&ls->l[L_PROFILE], profile, &line);

		if (rc == -1 || rc == -2) {
			fprintf(stderr, "rros-config: perfil '%s' no encontrado en %s\n", profile, rros_profile_dir());
			return -1;
		}
		if (rc) {
			fprintf(stderr, "rros-config: perfil '%s':%d: error de sintaxis\n", profile, line);
			return -1;
		}
		if (load_optional(&ls->l[L_SYSTEM], rros_system_conf()))
			return -1;
		if (rros_user_conf(user, sizeof user) == 0 && load_optional(&ls->l[L_USER], user))
			return -1;
	}
	for (int i = 0; i < 3; i++)
		for (size_t j = 0; j < ls->l[i].n; j++)
			if (rros_conf_set(&ls->merged, ls->l[i].items[j].key, ls->l[i].items[j].val,
					  ls->l[i].items[j].quoted))
				return -1;
	return 0;
}

static void free_layers(struct layers *ls)
{
	for (int i = 0; i < 3; i++)
		rros_conf_free(&ls->l[i]);
	rros_conf_free(&ls->merged);
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
		const char *paths[2] = { rros_system_conf(), user };
		struct rros_conf c;

		if (rros_user_conf(user, sizeof user) != 0)
			paths[1] = NULL;
		for (int i = 0; i < 2; i++) {
			if (!paths[i])
				continue;
			rros_conf_init(&c);
			if (load_optional(&c, paths[i]))
				bad++;
			else
				bad += rros_conf_validate(&c, 0, stderr, paths[i]);
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
			bad += rros_conf_validate(&c, profile_file, stderr, file);
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
	for (int f = RROS_FAM_KEYBIND; f <= RROS_FAM_AUTOSTART; f++) {
		static const char *names[] = { "", "keybind.<id>", "rule.<id>", "autostart.<id>" };

		if (markdown)
			printf("| `%s` | texto | libre, validado | %s |\n", names[f], rros_family_desc((enum rros_family)f));
		else
			printf("%-28s %-9s %-34s %s\n", names[f], "texto", "(clave dinámica)", rros_family_desc((enum rros_family)f));
	}
	return 0;
}

static int cmd_set(int argc, char **argv, int unset)
{
	int system = 0, a = 0, need = unset ? 1 : 2;
	char err[300];

	if (argc > 0 && !strcmp(argv[0], "--system")) {
		system = 1;
		a = 1;
	}
	if (argc - a != need) {
		fprintf(stderr, "uso: rros-config %s [--system] CLAVE%s\n", unset ? "unset" : "set", unset ? "" : " VALOR");
		fputs("     (¿el valor lleva espacios o #? ponlo entre comillas: 'Mod+b exec firefox', '#22d3ee')\n", stderr);
		return 1;
	}
	if (unset ? rros_user_unset(argv[a], system, err, sizeof err) :
		    rros_user_set(argv[a], argv[a + 1], system, err, sizeof err)) {
		fprintf(stderr, "rros-config: %s\n", err);
		return 1;
	}
	return 0;
}

static int cmd_export(void)
{
	char user[1024];
	FILE *f;
	int c;

	if (rros_user_conf(user, sizeof user))
		return 1;
	f = fopen(user, "r");
	if (!f)
		return 0; /* sin ajustes: salida vacía */
	while ((c = fgetc(f)) != EOF)
		putchar(c);
	fclose(f);
	return 0;
}

static int cmd_snapshot(int argc, char **argv)
{
	char err[300];

	if (argc == 1 && !strcmp(argv[0], "list")) {
		rros_snapshot_list(stdout);
		return 0;
	}
	if (argc != 2)
		return 2;
	if (!strcmp(argv[0], "save") ? rros_snapshot_save(argv[1], err, sizeof err) :
	    !strcmp(argv[0], "restore") ? rros_snapshot_restore(argv[1], err, sizeof err) :
	    !strcmp(argv[0], "delete") ? rros_snapshot_delete(argv[1], err, sizeof err) :
	    (snprintf(err, sizeof err, "subcomando desconocido '%s'", argv[0]), -1)) {
		fprintf(stderr, "rros-config: %s\n", err);
		return 1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	char profile[64] = "", err[300];
	const char *only_file = NULL;
	int i = 1, rc = 0;
	struct layers ls;
	const char *cmd;

	for (; i < argc && argv[i][0] == '-'; i++) {
		if (!strcmp(argv[i], "-p") && i + 1 < argc)
			snprintf(profile, sizeof profile, "%s", argv[++i]);
		else if (!strcmp(argv[i], "-f") && i + 1 < argc)
			only_file = argv[++i];
		else
			break;
	}
	if (i >= argc || !strcmp(argv[i], "help") || !strcmp(argv[i], "--help")) {
		usage(i >= argc ? stderr : stdout);
		return i >= argc ? 2 : 0;
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
	else if (!strcmp(cmd, "export") && i == argc)
		rc = cmd_export();
	else if (!strcmp(cmd, "import") && i + 1 == argc) {
		if (rros_user_import(argv[i], err, sizeof err)) {
			fprintf(stderr, "rros-config: %s\n", err);
			rc = 1;
		}
	} else if (!strcmp(cmd, "reset") && i + 1 == argc && !strcmp(argv[i], "--yes")) {
		if (rros_user_reset(err, sizeof err)) {
			fprintf(stderr, "rros-config: %s\n", err);
			rc = 1;
		}
	} else if (!strcmp(cmd, "reset")) {
		fputs("rros-config: reset borra todos tus ajustes (queda una copia .bak). Repite con --yes.\n", stderr);
		rc = 1;
	} else if (!strcmp(cmd, "snapshot"))
		rc = cmd_snapshot(argc - i, argv + i);
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
		usage(stderr);
		return 2;
	}
	if (rc == 2)
		usage(stderr);
	return rc;
}
