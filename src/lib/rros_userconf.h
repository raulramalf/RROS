/* Escritura validada de los ajustes del usuario/sistema, instantáneas y exportación. */
#ifndef RROS_USERCONF_H
#define RROS_USERCONF_H

#include <stdio.h>
#include <stddef.h>
#include "rros_conf.h"

/* 0 ok; <0 error con mensaje legible en err. `system`: escribe en /etc en vez de ~/.config. */
int rros_user_set(const char *key, const char *val, int system, char *err, size_t en);
int rros_user_unset(const char *key, int system, char *err, size_t en);
/* Solo comprueba que `key = val` se podría guardar (mismas reglas que rros_user_set). */
int rros_user_check(const char *key, const char *val, char *err, size_t en);

/* Valida todas las claves de una configuración (no de perfil). Devuelve nº de errores;
 * si `out` no es NULL, escribe cada error en una línea. */
int rros_conf_validate(const struct rros_conf *c, int profile_file, FILE *out, const char *name);

/* Instantáneas en <dir de rros.toml>/snapshots/NOMBRE.toml. */
int rros_snapshot_save(const char *name, char *err, size_t en);
int rros_snapshot_restore(const char *name, char *err, size_t en);
int rros_snapshot_delete(const char *name, char *err, size_t en);
/* Escribe un nombre por línea en `out`. Devuelve el nº de instantáneas. */
int rros_snapshot_list(FILE *out);

/* Sustituye los ajustes del usuario por los de `path` si son válidos (guarda copia .bak). */
int rros_user_import(const char *path, char *err, size_t en);
/* Vuelve a los valores del perfil (guarda copia .bak). */
int rros_user_reset(char *err, size_t en);

#endif
