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
/* 1 si la clave la aplica rros-apply como root (red, servicios, seguridad...): se guarda en
 * /etc/rros/rros.toml, no en la configuración del usuario. */
int rros_key_system_scope(const char *key);
/* idiomas tipo es_ES.UTF-8 / C.UTF-8 y zonas tipo Europe/Madrid (sin rutas peligrosas) */
int rros_locale_name_ok(const char *v);
int rros_timezone_name_ok(const char *v);
/* 0 ok; <0 con mensaje en err */
int rros_schema_check(const struct rros_key *k, const char *val, int quoted,
		      char *err, size_t n);


/* Claves dinámicas: keybind.<id>, rule.<id>, autostart.<id>. */
enum rros_family { RROS_FAM_NONE, RROS_FAM_KEYBIND, RROS_FAM_RULE, RROS_FAM_AUTOSTART };
enum rros_family rros_family_of(const char *key);
const char *rros_family_desc(enum rros_family f);
/* 1 si `key` no es dinámica; 0 si es válida; <0 con mensaje en err. */
int rros_dynamic_check(const char *key, const char *val, char *err, size_t n);
/* Atajo "Mod+Shift+b comando" -> combinación de KDE ("Meta+Shift+B") y comando. 0 ok, -1 inválido. */
int rros_keybind_parse(const char *val, char *combo, size_t cn, char *cmd, size_t mn);
/* Regla de ventana "app=NOMBRE ACCIÓN [N]" para KWin. 0 ok, -1 inválida. */
struct rros_rule {
	char match[64];   /* clase de la ventana o título */
	int by_title;
	char action[32];  /* above, fullscreen, maximized, noborder, skiptaskbar, minimized, opacity */
	int opacity;      /* 10-100 */
};
int rros_rule_parse(const char *val, struct rros_rule *r);

#endif
