/* Esquema de claves de configuración: tipo, rango, valores y descripción. */
#ifndef RROS_SCHEMA_H
#define RROS_SCHEMA_H

#include <stddef.h>

enum rros_type { RROS_T_STR, RROS_T_INT, RROS_T_BOOL, RROS_T_ENUM };

struct rros_key {
	const char *key;
	enum rros_type type;
	long min, max;          /* RROS_T_INT */
	const char *choices;    /* RROS_T_ENUM: "a|b|c" */
	int profile_only;       /* solo válida en archivos de perfil */
	const char *desc;
};

extern const struct rros_key rros_schema[];
extern const size_t rros_schema_len;

const struct rros_key *rros_schema_find(const char *key);
const char *rros_type_name(enum rros_type t);
/* 0 ok; <0 con mensaje en err */
int rros_schema_check(const struct rros_key *k, const char *val, int quoted,
		      char *err, size_t n);

#endif
