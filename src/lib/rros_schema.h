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


/* Claves dinámicas: keybind.<id>, rule.<id>, autostart.<id>. */
enum rros_family { RROS_FAM_NONE, RROS_FAM_KEYBIND, RROS_FAM_RULE, RROS_FAM_AUTOSTART };
enum rros_family rros_family_of(const char *key);
const char *rros_family_desc(enum rros_family f);
/* 1 si `key` no es dinámica; 0 si es válida; <0 con mensaje en err. */
int rros_dynamic_check(const char *key, const char *val, char *err, size_t n);
/* Traducción a comandos de sway (misma lógica que valida). 0 ok, -1 inválido. */
int rros_keybind_to_sway(const char *val, char *out, size_t n);
int rros_rule_to_sway(const char *val, char *out, size_t n);
int rros_autostart_to_sway(const char *val, char *out, size_t n);

#endif
