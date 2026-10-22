/* rros-settings: modelo común de la versión de terminal y de la aplicación Lily Ajustes. */
#ifndef RROS_SETTINGS_H
#define RROS_SETTINGS_H

#include <stddef.h>
#include "rros_conf.h"
#include "rros_schema.h"

struct model {
	struct rros_conf eff, prof, sys, user;
	char profile[64];
};

int model_load(struct model *m);          /* 0 ok */
void model_free(struct model *m);
/* "profile", "system" o "user": de dónde sale el valor efectivo de la clave. */
const char *model_source(const struct model *m, const char *key);

/* Categorías (claves estáticas agrupadas por prefijo) en orden estable. */
size_t model_categories(char names[][16], size_t max);
const char *model_category_label(const char *cat);

/* Buffer de texto dinámico. */
struct sbuf { char *p; size_t n, cap; };
void sb_free(struct sbuf *b);
int sb_put(struct sbuf *b, const char *s);
int sb_printf(struct sbuf *b, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
int sb_json(struct sbuf *b, const char *s);

/* Estado completo para la GUI, como JSON. 0 ok. */
int model_state_json(const struct model *m, struct sbuf *out);
/* Sistema live (la ISO, boot=casper; RROS_LIVE=1|0 lo fuerza en las pruebas). */
int model_is_live(void);

/* Instalación desde la GUI (install.c). */
struct install_req {
	char disk[64], mode[16], user[40], fullname[96], host[72], keymap[24], locale[24], tz[48];
	char password[256], passphrase[256];
	char cmdline[128];       /* parámetros extra del kernel (solo por la API: pruebas en QEMU) */
	int encrypt;
};
int install_start(const struct install_req *r, char *err, size_t en);   /* 0 si se lanzó */
int install_running(void);
void install_status_json(struct sbuf *b);

/* Utilidades de proceso. */
void tool_dir_from_argv0(const char *argv0);
int run_tool(const char *tool, char *const argv[], int quiet);   /* devuelve código de salida */
int run_capture(const char *tool, char *const argv[], struct sbuf *out);   /* guarda la salida estándar */
int run_capture_all(const char *tool, char *const argv[], struct sbuf *out);   /* salida y errores */
int spawn_detached(const char *tool, char *const argv[]);               /* 0 si se lanzó */

/* Modos. */
int run_tui(int auto_apply);
/* rros-settings api MÉTODO RUTA [CUERPO]: responde en JSON por la salida estándar. 0 si fue bien. */
int run_api(const char *method, const char *target, const char *body);

#endif
