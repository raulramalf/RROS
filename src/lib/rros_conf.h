/* Parser/escritor de un subconjunto de TOML: [seccion], clave = valor. */
#ifndef RROS_CONF_H
#define RROS_CONF_H

#include <stddef.h>

struct rros_kv {
	char *key;   /* "seccion.clave" o "clave" */
	char *val;   /* sin comillas */
	int quoted;  /* 1 si se escribió como cadena */
};

struct rros_conf {
	struct rros_kv *items;
	size_t n, cap;
};

void rros_conf_init(struct rros_conf *c);
void rros_conf_free(struct rros_conf *c);

/* 0 ok, -1 no se pudo abrir (errno), -2 error de sintaxis (*errline), -3 sin memoria */
int rros_conf_parse(struct rros_conf *c, const char *text, int *errline);
int rros_conf_load(struct rros_conf *c, const char *path, int *errline);

const char *rros_conf_get(const struct rros_conf *c, const char *key);
long rros_conf_get_long(const struct rros_conf *c, const char *key, long def);
int rros_conf_get_bool(const struct rros_conf *c, const char *key, int def);

/* 0 ok, -1 sin memoria */
int rros_conf_set(struct rros_conf *c, const char *key, const char *val, int quoted);
int rros_conf_unset(struct rros_conf *c, const char *key);

void rros_conf_sort(struct rros_conf *c);
/* Escritura atómica (tmp + rename). 0 ok, <0 error. */
int rros_conf_save(struct rros_conf *c, const char *path);

#endif
