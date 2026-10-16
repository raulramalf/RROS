#include "rros_userconf.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "rros_profile.h"
#include "rros_schema.h"

static int conf_path(int system, char *buf, size_t n)
{
	if (system)
		return snprintf(buf, n, "%s", rros_system_conf()) >= (int)n ? -1 : 0;
	return rros_user_conf(buf, n);
}

static int load_for_edit(struct rros_conf *c, const char *path, char *err, size_t en)
{
	int line = 0, rc = rros_conf_load(c, path, &line);

	if (rc == 0 || (rc == -1 && errno == ENOENT))
		return 0;
	snprintf(err, en, "%s está ilegible o tiene errores (línea %d)", path, line);
	return -1;
}

static int is_quoted_type(const struct rros_key *k)
{
	return k->type == RROS_T_STR || k->type == RROS_T_ENUM;
}

int rros_user_set(const char *key, const char *val, int system, char *err, size_t en)
{
	const struct rros_key *k = rros_schema_find(key);
	struct rros_conf c;
	char path[1024];
	int quoted, rc;

	if (k) {
		if (k->profile_only) {
			snprintf(err, en, "'%s' solo es válida en archivos de perfil", key);
			return -1;
		}
		if (rros_schema_check(k, val, is_quoted_type(k), err, en))
			return -1;
		quoted = is_quoted_type(k);
	} else {
		rc = rros_dynamic_check(key, val, err, en);
		if (rc > 0) {
			snprintf(err, en, "clave '%s' desconocida (ver `rros-config schema`)", key);
			return -1;
		}
		if (rc < 0)
			return -1;
		quoted = 1;
	}
	if (conf_path(system, path, sizeof path)) {
		snprintf(err, en, "no se pudo determinar el archivo de configuración");
		return -1;
	}
	rros_conf_init(&c);
	if (load_for_edit(&c, path, err, en)) {
		rros_conf_free(&c);
		return -1;
	}
	if (rros_conf_set(&c, key, val, quoted) || rros_mkdir_parents(path) || rros_conf_save(&c, path)) {
		snprintf(err, en, "no se pudo escribir %s: %s", path, strerror(errno));
		rros_conf_free(&c);
		return -1;
	}
	rros_conf_free(&c);
	return 0;
}

int rros_user_unset(const char *key, int system, char *err, size_t en)
{
	struct rros_conf c;
	char path[1024];

	if (!rros_schema_find(key) && rros_family_of(key) == RROS_FAM_NONE) {
		snprintf(err, en, "clave '%s' desconocida (ver `rros-config schema`)", key);
		return -1;
	}
	if (conf_path(system, path, sizeof path)) {
		snprintf(err, en, "no se pudo determinar el archivo de configuración");
		return -1;
	}
	rros_conf_init(&c);
	if (load_for_edit(&c, path, err, en)) {
		rros_conf_free(&c);
		return -1;
	}
	rros_conf_unset(&c, key);
	if (rros_mkdir_parents(path) || rros_conf_save(&c, path)) {
		snprintf(err, en, "no se pudo escribir %s", path);
		rros_conf_free(&c);
		return -1;
	}
	rros_conf_free(&c);
	return 0;
}

int rros_conf_validate(const struct rros_conf *c, int profile_file, FILE *out, const char *name)
{
	char err[256];
	int bad = 0;

	for (size_t i = 0; i < c->n; i++) {
		const struct rros_kv *kv = &c->items[i];
		const struct rros_key *k = rros_schema_find(kv->key);
		int dyn;

		if (!k) {
			dyn = rros_dynamic_check(kv->key, kv->val, err, sizeof err);
			if (dyn > 0)
				snprintf(err, sizeof err, "clave desconocida '%s'", kv->key);
			if (dyn == 0)
				continue;
		} else if (k->profile_only && !profile_file) {
			snprintf(err, sizeof err, "'%s' solo es válida en archivos de perfil", kv->key);
		} else if (rros_schema_check(k, kv->val, kv->quoted, err, sizeof err)) {
			/* err ya tiene el mensaje */
		} else {
			continue;
		}
		if (out)
			fprintf(out, "%s: %s\n", name, err);
		bad++;
	}
	return bad;
}

/* ---- instantáneas ---- */

static int snap_dir(char *buf, size_t n)
{
	char user[1024], *slash;

	if (rros_user_conf(user, sizeof user))
		return -1;
	slash = strrchr(user, '/');
	if (!slash)
		return -1;
	*slash = '\0';
	return snprintf(buf, n, "%s/snapshots", user) >= (int)n ? -1 : 0;
}

static int snap_path(const char *name, char *buf, size_t n, char *err, size_t en)
{
	char dir[900];

	if (!rros_valid_profile_name(name)) {
		snprintf(err, en, "nombre de instantánea no válido (a-z, 0-9, _ o -, máx. 32)");
		return -1;
	}
	if (snap_dir(dir, sizeof dir)) {
		snprintf(err, en, "no se pudo determinar el directorio de instantáneas");
		return -1;
	}
	return snprintf(buf, n, "%s/%s.toml", dir, name) >= (int)n ? -1 : 0;
}

int rros_snapshot_save(const char *name, char *err, size_t en)
{
	char path[1100], user[1024];
	struct rros_conf c;
	int rc, line = 0;

	if (snap_path(name, path, sizeof path, err, en) || rros_user_conf(user, sizeof user))
		return -1;
	rros_conf_init(&c);
	rc = rros_conf_load(&c, user, &line);
	if (rc && !(rc == -1 && errno == ENOENT)) {
		snprintf(err, en, "tus ajustes están ilegibles");
		rros_conf_free(&c);
		return -1;
	}
	if (rros_mkdir_parents(path) || rros_conf_save(&c, path)) {
		snprintf(err, en, "no se pudo escribir %s", path);
		rros_conf_free(&c);
		return -1;
	}
	rros_conf_free(&c);
	return 0;
}

int rros_user_import(const char *path, char *err, size_t en)
{
	struct rros_conf c;
	char user[1024], bak[1040];
	int line = 0, rc;

	if (rros_user_conf(user, sizeof user)) {
		snprintf(err, en, "no se pudo determinar el archivo de usuario");
		return -1;
	}
	rros_conf_init(&c);
	rc = rros_conf_load(&c, path, &line);
	if (rc == -1) {
		snprintf(err, en, "%s: %s", path, strerror(errno));
		rros_conf_free(&c);
		return -1;
	}
	if (rc) {
		snprintf(err, en, "%s:%d: error de sintaxis", path, line);
		rros_conf_free(&c);
		return -1;
	}
	if (rros_conf_validate(&c, 0, NULL, path)) {
		snprintf(err, en, "%s tiene valores no válidos (usa `rros-config validate %s`)", path, path);
		rros_conf_free(&c);
		return -1;
	}
	snprintf(bak, sizeof bak, "%s.bak", user);
	if (access(user, F_OK) == 0 && rename(user, bak) != 0) {
		snprintf(err, en, "no se pudo guardar la copia de seguridad");
		rros_conf_free(&c);
		return -1;
	}
	if (rros_mkdir_parents(user) || rros_conf_save(&c, user)) {
		snprintf(err, en, "no se pudo escribir %s", user);
		rros_conf_free(&c);
		return -1;
	}
	rros_conf_free(&c);
	return 0;
}

int rros_snapshot_restore(const char *name, char *err, size_t en)
{
	char path[1100];

	if (snap_path(name, path, sizeof path, err, en))
		return -1;
	if (access(path, R_OK) != 0) {
		snprintf(err, en, "no existe la instantánea '%s'", name);
		return -1;
	}
	return rros_user_import(path, err, en);
}

int rros_snapshot_delete(const char *name, char *err, size_t en)
{
	char path[1100];

	if (snap_path(name, path, sizeof path, err, en))
		return -1;
	if (remove(path) != 0) {
		snprintf(err, en, "no existe la instantánea '%s'", name);
		return -1;
	}
	return 0;
}

int rros_snapshot_list(FILE *out)
{
	char dir[1024];
	struct dirent *e;
	DIR *d;
	int n = 0;

	if (snap_dir(dir, sizeof dir))
		return 0;
	d = opendir(dir);
	if (!d)
		return 0;
	while ((e = readdir(d))) {
		size_t l = strlen(e->d_name);

		if (l > 5 && !strcmp(e->d_name + l - 5, ".toml")) {
			fprintf(out, "%.*s\n", (int)(l - 5), e->d_name);
			n++;
		}
	}
	closedir(d);
	return n;
}

int rros_user_reset(char *err, size_t en)
{
	char user[1024], bak[1040];

	if (rros_user_conf(user, sizeof user)) {
		snprintf(err, en, "no se pudo determinar el archivo de usuario");
		return -1;
	}
	if (access(user, F_OK) != 0)
		return 0;
	snprintf(bak, sizeof bak, "%s.bak", user);
	if (rename(user, bak) != 0) {
		snprintf(err, en, "no se pudo mover %s", user);
		return -1;
	}
	return 0;
}
