/* Rutas, capas de configuración (perfil < sistema < usuario) y perfiles. */
#ifndef RROS_PROFILE_H
#define RROS_PROFILE_H

#include <stddef.h>
#include "rros_conf.h"

const char *rros_profile_dir(void);     /* RROS_PROFILE_DIR | /usr/share/rros/profiles */
const char *rros_system_conf(void);     /* RROS_SYSTEM_CONF | /etc/rros/rros.toml */
const char *rros_profile_file(void);    /* RROS_PROFILE_FILE | /etc/rros/profile */
/* RROS_USER_CONF | $XDG_CONFIG_HOME/rros/rros.toml | $HOME/.config/rros/rros.toml */
int rros_user_conf(char *buf, size_t n);

int rros_valid_profile_name(const char *name);
/* 0 ok, -1 nombre inválido, -2 no existe/ilegible, -3 error de sintaxis (*errline) */
int rros_load_profile(struct rros_conf *c, const char *name, int *errline);
/* Perfil activo: contenido de rros_profile_file(), o "standard". */
int rros_read_active_profile(char *buf, size_t n);
int rros_write_active_profile(const char *name);
/* Crea los directorios padre de `path`. */
int rros_mkdir_parents(const char *path);

#endif
